/* Included once by calibration.c: owns the analog mux through the same
 * exclusive-channel helper as diagnostics. The pure controller is host tested. */
#include "control_runtime.h"
#include "control_config.c"

static ControlConfig ctl_config, ctl_staged, ctl_profile_config;
static ControlPI ctl_pi;
static uint8 ctl_state, ctl_profile, ctl_origin, ctl_channel=4u, ctl_active;
static uint8 ctl_selected, ctl_bytes, ctl_band, ctl_scan, ctl_phase;
static uint8 ctl_learning_refinements;
static int16 ctl_learning_slow_step;
static uint8 ctl_learning_have_pos,ctl_learning_have_neg;
static int16 ctl_learning_pos0,ctl_learning_pos1,ctl_learning_neg0,ctl_learning_neg1;
static uint8 ctl_learning_had_profile;
static uint32 ctl_word, ctl_last_report, ctl_last_scan, ctl_last_pi;
static uint32 ctl_learning_start, ctl_phase_start, ctl_stable_start, ctl_enter_start;
static uint32 ctl_revision, ctl_generation;
static int16 ctl_codes[4], ctl_saved_codes[4], ctl_learning_restore[4];
static int32 ctl_dc[5], ctl_anchor[5];
static uint32 ctl_stamp[5];
static uint8 ctl_valid[5], ctl_seen[5];
static volatile uint32 ctl_ms;
static volatile int64_t ctl_sum;
static volatile uint16 ctl_count, ctl_discard;
static volatile uint8 ctl_ready;
static volatile int32 ctl_average;
static uint8 ctl_nv_slot;
static uint8 ctl_image[256];
static uint8 ctl_initialized;
static uint8 ctl_capture_metadata[CONTROL_CAPTURE_METADATA_BYTES];
/* LPo trace: every fresh LPo average (~20 ms) with its timestamp, to find
 * periodic disturbances without an oscilloscope.  `ctl get 255` dumps it. */
#define CTL_TRACE_LEN 127u
static int32 ctl_trace_v[CTL_TRACE_LEN];
static uint32 ctl_trace_ms[CTL_TRACE_LEN];
static uint8 ctl_trace_head, ctl_trace_full;
static uint32 ctl_report_start, ctl_quiet_until;
/* Median of the last 5 averages per channel: the chain shows 40 ms dips of
 * -30 mV every ~2 s (PSoC trace, 2026-09-16) that an average cannot remove. */
#define CTL_MEDIAN_LEN 5u
static int32 ctl_med[CTL_MEDIAN_LEN];
static uint8 ctl_med_n, ctl_med_i;
static int32 ctl_median(void)
{
    int32 t[CTL_MEDIAN_LEN],x;uint8 i,j;
    memcpy(t,ctl_med,sizeof t);
    for(i=1;i<CTL_MEDIAN_LEN;++i){x=t[i];for(j=i;j>0&&t[j-1]>x;--j)t[j]=t[j-1];t[j]=x;}
    return t[CTL_MEDIAN_LEN/2u];
}
static uint16 ctl_report_count;
void control_copy_capture_metadata(uint8 *out)
{ memcpy(out,ctl_capture_metadata,CONTROL_CAPTURE_METADATA_BYTES); }

void control_tick(void) { ++ctl_ms; }
uint32 control_millis(void) { return ctl_ms; }
static void ctl_put32(uint8 *p,uint32 x)
{ p[0]=(uint8)x;p[1]=(uint8)(x>>8);p[2]=(uint8)(x>>16);p[3]=(uint8)(x>>24); }
static uint32 ctl_get32(const uint8 *p)
{ return (uint32)p[0]|((uint32)p[1]<<8)|((uint32)p[2]<<16)|((uint32)p[3]<<24); }
static uint8 ctl_load_slot(uint8 slot)
{
    uint16 i,base=(uint16)(256u+256u*slot);
    for(i=0;i<256;++i) ctl_image[i]=EEPROM_ReadByte(base+i);
    return ctl_image[0]==0x47 && ctl_image[1]==0x43 &&
        (ctl_image[2]==3u || ctl_image[2]==CONTROL_NV_VERSION) && ctl_image[3]==PSOC_HW_CLASS &&
        control_crc(ctl_image,254)==((uint16)ctl_image[254]|((uint16)ctl_image[255]<<8));
}
static uint8 ctl_store(void)
{
    uint8 slot=(uint8)(1u-ctl_nv_slot),row;
    uint16 crc,i;
    memset(ctl_image,0,sizeof ctl_image);
    ctl_image[0]=0x47;ctl_image[1]=0x43;ctl_image[2]=CONTROL_NV_VERSION;ctl_image[3]=PSOC_HW_CLASS;
    ctl_put32(ctl_image+4,ctl_generation+1);
    control_config_encode(&ctl_config,ctl_image+8);
    i=8+CONTROL_CONFIG_BYTES;
    ctl_image[i++]=ctl_profile;
    for(row=0;row<4;++row) { ctl_image[i++]=(uint8)ctl_saved_codes[row];ctl_image[i++]=(uint8)((uint16)ctl_saved_codes[row]>>8); }
    ctl_put32(ctl_image+i,ctl_revision);
    crc=control_crc(ctl_image,254);ctl_image[254]=(uint8)crc;ctl_image[255]=(uint8)(crc>>8);
    EEPROM_UpdateTemperature();
    /* Invalidate destination before payload. Commit header LAST. Old slot
     * remains untouched on every failure or interrupted write. */
    { uint8 zero[16]={0}; if(EEPROM_Write(zero,(uint8)(16+slot*16))!=CYRET_SUCCESS) return 0; }
    for(row=1;row<16;++row)
        if(EEPROM_Write(ctl_image+16*row,(uint8)(16+slot*16+row))!=CYRET_SUCCESS) return 0;
    if(EEPROM_Write(ctl_image,(uint8)(16+slot*16))!=CYRET_SUCCESS) return 0;
    if(!ctl_load_slot(slot)) return 0;
    ctl_nv_slot=slot; ++ctl_generation;ctl_origin=1;return 1;
}
static void ctl_emit(uint16 key,int32 value,uint8 capture)
{
    uint8 f[11],i,x=0;
    f[0]=0xAB;f[1]=0xC6;f[2]=CONTROL_CONFIG_VERSION;f[3]=capture;
    f[4]=(uint8)key;f[5]=(uint8)(key>>8);ctl_put32(f+6,(uint32)value);
    for(i=2;i<10;++i)x^=f[i];f[10]=x;psoc_link_put_array(f,11);
    ctl_quiet_until=ctl_ms+(uint32)ctl_config.value[CP_QUIET_MS];
}
static void ctl_emit_trace(void)
{
    uint8 i,n=ctl_trace_full?CTL_TRACE_LEN:ctl_trace_head;
    uint8 idx=ctl_trace_full?ctl_trace_head:0u;
    uint32 prev=0;
    ctl_emit(0x17D,(int32)ctl_report_start,0);
    ctl_emit(0x17E,(int32)ctl_last_report,0);
    ctl_emit(0x17F,(int32)(n?ctl_trace_ms[(uint8)((idx+n-1u)%CTL_TRACE_LEN)]:0),0);
    for(i=0;i<n;++i) {
        uint8 k=(uint8)((idx+i)%CTL_TRACE_LEN);
        uint32 dt=i?ctl_trace_ms[k]-prev:0u;
        int32 mv10=ctl_trace_v[k]/10;
        if(dt>4095u)dt=4095u;
        if(mv10>524287)mv10=524287;
        if(mv10<-524288)mv10=-524288;
        prev=ctl_trace_ms[k];
        ctl_emit((uint16)(0x180+i),(int32)((dt<<20)|((uint32)mv10&0xFFFFFu)),0);
    }
}
void control_emit_snapshot(uint8 capture)
{
    uint16 i;uint32 now=ctl_ms;
    /* The 36 configuration words only change on apply: sending them every
     * report was 36 of 72 I2C frames per second for nothing. */
    uint8 with_config=capture || (ctl_report_count%30u)==0u;
    ++ctl_report_count;
    ctl_emit(0x100,ctl_state,capture);ctl_emit(0x101,ctl_profile,capture);
    ctl_emit(0x102,ctl_band,capture);ctl_emit(0x103,ctl_origin,capture);
    ctl_emit(0x104,ctl_revision,capture);ctl_emit(0x105,(int32)now,capture);
    ctl_emit(0x106,CONTROL_CONFIG_VERSION,capture);
    ctl_emit(0x107,control_crc((const uint8 *)(__DATE__ " " __TIME__),sizeof(__DATE__ " " __TIME__)-1),capture);
    if(with_config)for(i=0;i<CP_COUNT;++i)ctl_emit(i,ctl_config.value[i],capture);
    for(i=0;i<4;++i)ctl_emit((uint16)(0x110+i),ctl_codes[i],capture);
    for(i=0;i<5;++i) {
        int32 margin=ctl_dc[i]-(ctl_config.value[CP_VALID_LOW_UV]+ctl_config.value[CP_MARGIN_UV]);
        int32 upper=(ctl_config.value[CP_VALID_HIGH_UV]-ctl_config.value[CP_MARGIN_UV])-ctl_dc[i];
        if(upper<margin)margin=upper;
        ctl_emit((uint16)(0x120+i*4),ctl_dc[i],capture);
        ctl_emit((uint16)(0x121+i*4),ctl_seen[i]?ctl_valid[i]:0,capture);
        ctl_emit((uint16)(0x122+i*4),ctl_seen[i]?(int32)(now-ctl_stamp[i]):-1,capture);
        ctl_emit((uint16)(0x123+i*4),ctl_seen[i]?margin:INT32_MIN,capture);
    }
    ctl_emit(0x1ff,1,capture);
}
static void ctl_select(uint8 channel)
{
    uint8 saved;
    ADC_StopConvert();ctl_active=0;
    psoc_amux_select_exclusive(channel,(uint8)ctl_config.value[CP_CAPACITOR]);
    saved=CyEnterCriticalSection();
    ctl_channel=channel;ctl_sum=0;ctl_count=0;ctl_ready=0;ctl_med_n=0;ctl_med_i=0;
    ctl_discard=(uint16)ctl_config.value[CP_SETTLE_SAMPLES];
    ctl_active=1;CyExitCriticalSection(saved);ADC_StartConvert();
}
void control_on_filtered_sample(int32 counts)
{
    if(!ctl_active)return;
    if(ctl_discard){--ctl_discard;return;}
    ctl_sum+=counts;
    if(++ctl_count >= (uint16)ctl_config.value[CP_AVERAGE]) {
        ctl_average=(int32)(ctl_sum/ctl_count);ctl_ready=1;
        ctl_sum=0;ctl_count=0;
    }
}
static void ctl_route_start(void)
{
    ADC_StopConvert();psoc_adc_select_calibration_config();
    (void)psoc_filter_load_fir_coefficients(g_fir_calibration_coeffs_q23,FILTER_FIR_NTAPS);
    dma_route_select(1u);ctl_scan=1;ctl_select(0);
}
static void ctl_write(uint8 stage,int16 code)
{
    if(ctl_state==CONTROL_PAUSED)return;
    if(psoc_calibration_set_stage_dac(stage,code))ctl_codes[stage]=code;
}
uint8 control_capture_channel(void) { return (uint8)ctl_config.value[CP_CAPTURE_CHANNEL]; }
void control_select_capture_path(void)
{ if(ctl_initialized)psoc_amux_select_exclusive(control_capture_channel(),0); }
void control_prepare_capture(void)
{
    uint16 pos;uint8 j;
    if(!ctl_initialized)return;
    ctl_active=0;ADC_StopConvert();
    if(ctl_state==CONTROL_LEARNING) {
        uint8 i;
        /* Interrupted learning is never persisted; restore the last profile. */
        for(i=0;i<4;++i)ctl_write(i,ctl_learning_restore[i]);
    }
    control_emit_snapshot(1);
    memset(ctl_capture_metadata,0,sizeof ctl_capture_metadata);
    ctl_capture_metadata[0]='C';ctl_capture_metadata[1]='T';ctl_capture_metadata[2]=CONTROL_CONFIG_VERSION;
    ctl_capture_metadata[3]=ctl_profile;ctl_capture_metadata[4]=ctl_band;ctl_capture_metadata[5]=ctl_origin;
    ctl_put32(ctl_capture_metadata+8,ctl_revision);ctl_put32(ctl_capture_metadata+12,ctl_ms);
    control_config_encode(&ctl_config,ctl_capture_metadata+16);pos=16+CONTROL_CONFIG_BYTES;
    for(j=0;j<4;++j){ctl_put32(ctl_capture_metadata+pos,(uint32)(int32)ctl_codes[j]);pos+=4;}
    for(j=0;j<5;++j){ctl_put32(ctl_capture_metadata+pos,(uint32)ctl_dc[j]);pos+=4;
        ctl_put32(ctl_capture_metadata+pos,ctl_seen[j]?ctl_ms-ctl_stamp[j]:UINT32_MAX);pos+=4;
        ctl_put32(ctl_capture_metadata+pos,ctl_seen[j]&&ctl_valid[j]);pos+=4;}
    ctl_put32(ctl_capture_metadata+pos,control_crc((const uint8 *)(__DATE__ " " __TIME__),sizeof(__DATE__ " " __TIME__)-1));
    ctl_state=CONTROL_PAUSED;ctl_pi.has_previous=0;ctl_pi.fraction=0;
    psoc_amux_select_exclusive(control_capture_channel(),0);
    psoc_adc_cal_override(0);
}
void control_resume(void)
{
    if(!ctl_initialized)return;
    ctl_state=ctl_profile?CONTROL_RUNNING:CONTROL_NEEDS_LEARNING;
    ctl_pi.has_previous=0;ctl_pi.fraction=0;ctl_pi.holding=0;
    ctl_pi.fine=ctl_codes[3];ctl_pi.coarse=ctl_codes[2];
    ctl_route_start();
}
void control_init(void)
{
    uint8 a,b,i;uint32 ga=0,gb=0;uint16 pos;
    control_config_defaults(&ctl_config);
    a=ctl_load_slot(0);if(a)ga=ctl_get32(ctl_image+4);
    b=ctl_load_slot(1);if(b)gb=ctl_get32(ctl_image+4);
    ctl_nv_slot=(b && (!a || (int32)(gb-ga)>0))?1:0;
    if((a||b) && ctl_load_slot(ctl_nv_slot)) {
        uint8 legacy=ctl_image[2]==3u;
        uint16 count=legacy?CONTROL_CONFIG_COUNT_V3:CP_COUNT;
        control_config_decode_n(&ctl_staged,ctl_image+8,count);
        if(legacy) {
            /* v3 stored a fast-loop tuning in the wrong domain.  Keep what
             * belongs to the learned profile, take the fast loop from the
             * v4 defaults. */
            static const uint8 fast[]={CP_PERIOD_MS,CP_TAU_MS,CP_KP_NUM,CP_KP_DEN,
                CP_DEADBAND_UV,CP_FINE_STEP,CP_RESCUE_STEP,CP_RESCUE_MS,CP_SUM_BAND_UV,
                CP_COARSE_STEP,CP_FINE_SLOPE_UV,CP_COARSE_SLOPE_UV,CP_SCAN_MS,CP_REPORT_MS};
            ControlConfig d;control_config_defaults(&d);
            for(i=0;i<sizeof fast;++i)ctl_staged.value[fast[i]]=d.value[fast[i]];
        }
        if(control_config_valid(&ctl_staged,PSOC_AMUX_SIGNAL_CHANNEL_COUNT,CAL_AMUX_HAS_CAP_CHANNEL)) {
            ctl_config=ctl_staged;ctl_origin=1;ctl_generation=ctl_get32(ctl_image+4);
            pos=(uint16)(8u+count*4u);ctl_profile=ctl_image[pos++]==1;
            for(i=0;i<4;++i){ctl_saved_codes[i]=(int16)((uint16)ctl_image[pos]|((uint16)ctl_image[pos+1]<<8));pos+=2;
                if(ctl_saved_codes[i]<-255||ctl_saved_codes[i]>255)ctl_profile=0;}
            ctl_revision=ctl_get32(ctl_image+pos);
        }
    }
    ctl_staged=ctl_config;ctl_profile_config=ctl_config;
    psoc_hw_set_pga((uint8)ctl_config.value[CP_PGA]);psoc_hw_set_pgaout((uint8)ctl_config.value[CP_PGAOUT]);
    ctl_state=CONTROL_NEEDS_LEARNING;
    for(i=0;i<4;++i)ctl_write(i,ctl_profile?ctl_saved_codes[i]:0);
    ctl_initialized=1;CySysTickStart();CySysTickSetCallback(0,control_tick);
    control_resume();
}
/* Settled mode slows the diagnostic scan and the reports: both put ticks on
 * the analog chain (AMux switching, I2C bursts to the ESP). */
static uint8 ctl_settled(uint32 now)
{ return ctl_state==CONTROL_RUNNING && control_pi_settled(&ctl_pi,&ctl_config,now); }
static uint32 ctl_scan_ms(uint32 now)
{ return (uint32)ctl_config.value[ctl_settled(now)?CP_SCAN_SETTLED_MS:CP_SCAN_MS]; }
static uint8 ctl_all_healthy(uint32 now)
{
    uint8 i;
    for(i=0;i<5;++i)if(!ctl_seen[i]||!ctl_valid[i]||now-ctl_stamp[i]>ctl_scan_ms(now)*2u)return 0;
    return 1;
}
static void ctl_learning_fail(uint16 reason)
{
    uint8 i;
    for(i=0;i<4;++i) {
        ctl_write(i,ctl_learning_restore[i]);
        if(ctl_learning_had_profile)ctl_saved_codes[i]=ctl_learning_restore[i];
    }
    ctl_profile=ctl_learning_had_profile;
    ctl_state=ctl_profile?CONTROL_RUNNING:CONTROL_FAILED;
    ctl_pi.fine=ctl_codes[3];ctl_pi.coarse=ctl_codes[2];
    ctl_pi.has_previous=0;ctl_pi.fraction=0;
    ctl_emit(0x108,(int32)reason,0);
}
static uint8 ctl_choose_slow_codes(int *out0,int *out1)
{
    const int32 *v=ctl_config.value;
    int a,b,ba=ctl_codes[0],bb=ctl_codes[1];
    int32 k0=(int32)(-(int64)v[CP_SLOW0_SLOPE_UV]*270/68);
    int32 k1=(int32)(-(int64)v[CP_SLOW1_SLOPE_UV]*2700/748);
    int32 gain_x1000=(int32)psoc_hw_pgaout_gain_x1000();
    int32 s0=(int32)((int64)k0*gain_x1000/1000);
    int32 s1=(int32)((int64)k1*gain_x1000/1000);
    int64 best=INT64_MAX,best_cost=INT64_MAX;
    for(a=-255;a<=255;++a)for(b=-255;b<=255;++b) {
        int32 da=a-ctl_codes[0],db=b-ctl_codes[1];
        int64 predicted=(int64)ctl_dc[3]+(int64)da*s0+(int64)db*s1;
        int64 residual=magnitude(predicted-v[CP_OPA_TARGET_UV]);
        int64 cost=(int64)a*a+(int64)b*b;
        /* Solve the one SUMo error with both slow degrees of freedom, choosing
         * the lowest absolute IDAC effort among equivalent integer pairs. */
        if((residual<=v[CP_STABLE_UV] && (best>v[CP_STABLE_UV]||cost<best_cost)) ||
           (best>v[CP_STABLE_UV] && residual<best)) {
            best=residual;best_cost=cost;ba=a;bb=b;
        }
    }
    if(best==INT64_MAX)return 0;
    *out0=ba;*out1=bb;return 1;
}
static uint8 ctl_rescue_sum_with_slow_pair(uint32 now)
{
    const int32 *v=ctl_config.value;
    int32 k0=magnitude((int32)(-(int64)v[CP_SLOW0_SLOPE_UV]*270/68));
    int32 k1=magnitude((int32)(-(int64)v[CP_SLOW1_SLOPE_UV]*2700/748));
    int sign=ctl_dc[3]>=0?1:-1;
    int total,d0,d1,n0,n1;
    if(ctl_learning_slow_step<1) {
        ctl_learning_slow_step=(int16)(v[CP_RESCUE_STEP]/3);
        if(ctl_learning_slow_step<2)ctl_learning_slow_step=2;
    }
    if(sign>0) {
        ctl_learning_pos0=ctl_codes[0];ctl_learning_pos1=ctl_codes[1];
        ctl_learning_have_pos=1;
    } else {
        ctl_learning_neg0=ctl_codes[0];ctl_learning_neg1=ctl_codes[1];
        ctl_learning_have_neg=1;
    }
    if(ctl_learning_have_pos && ctl_learning_have_neg) {
        /* We know one code pair on each physical rail.  Bisect the codes, not
         * the clipped ADC numbers. */
        n0=(ctl_learning_pos0+ctl_learning_neg0)/2;
        n1=(ctl_learning_pos1+ctl_learning_neg1)/2;
    } else {
        total=ctl_learning_slow_step;
        d0=(int)((int64)total*k0/(k0+k1));
        if(d0<1)d0=1;
        d1=total-d0;if(d1<1)d1=1;
        /* Both modeled slopes are negative.  A positive rail therefore needs
         * positive codes; a negative rail needs negative codes. */
        n0=bounded(ctl_codes[0]+sign*d0,-255,255);
        n1=bounded(ctl_codes[1]+sign*d1,-255,255);
    }
    if(n0==ctl_codes[0] && n1==ctl_codes[1])return 0;
    ctl_write(0,(int16)n0);ctl_write(1,(int16)n1);
    ++ctl_learning_refinements;ctl_phase_start=now;
    return 1;
}
static void ctl_learning(uint32 now)
{
    const int32 *v=ctl_config.value;
    uint8 i;
    if(now-ctl_learning_start>(uint32)v[CP_LEARN_TIMEOUT_MS]) { ctl_learning_fail(1);return; }
    if(now-ctl_phase_start<(uint32)v[CP_SLOW_TAU_MS]*2u)return;
    if(ctl_phase==0 && ctl_seen[0] && ctl_seen[1] && ctl_seen[2] && ctl_seen[3]) {
        int ba,bb;
        /* A rail value carries a trustworthy sign but no trustworthy
         * magnitude.  Walk both slow controls in their maximum-effect
         * direction until SUMo becomes observable; never fit the clipped
         * number as though it were a voltage. */
        if(!control_measurement_valid(&ctl_config,ctl_dc[3])) {
            if(ctl_learning_refinements>=8u || !ctl_rescue_sum_with_slow_pair(now))
                ctl_learning_fail(2);
            return;
        }
        if(!ctl_choose_slow_codes(&ba,&bb)){ctl_learning_fail(2);return;}
        ctl_write(0,(int16)ba);ctl_write(1,(int16)bb);
        ctl_phase=1;ctl_phase_start=now;ctl_stable_start=now;
    } else if(ctl_phase==1) {
        /* IDAC0/1 feed the slow BP path.  Do not start the fast controller
         * from their pre-step samples: wait two measured time constants and
         * refine the integer pair if SUMo is still outside its truthful
         * window. */
        if(!ctl_seen[3])return;
        if(!control_measurement_valid(&ctl_config,ctl_dc[3])) {
            ctl_phase=0;
            if(ctl_learning_refinements>=8u || !ctl_rescue_sum_with_slow_pair(now))
                ctl_learning_fail(2);
            return;
        }
        ctl_phase=2;ctl_phase_start=now;ctl_stable_start=now;
        for(i=0;i<5;++i)ctl_anchor[i]=ctl_dc[i];
    } else if(ctl_phase==2) {
        if(!ctl_all_healthy(now)) {
            ctl_stable_start=now;return;
        }
        /* The learned profile belongs only to the slow pair (SEo/BPo) and its
         * objective OPA_SUMo.  SUMo/LPo are being actively moved by the fast
         * PI in this same phase, so requiring those two taps to remain still
         * creates an impossible completion condition. */
        for(i=0;i<3;++i)if(magnitude(ctl_dc[i]-ctl_anchor[i])>v[CP_STABLE_UV]) {
            uint8 j;ctl_stable_start=now;for(j=0;j<3;++j)ctl_anchor[j]=ctl_dc[j];return;
        }
        if(now-ctl_stable_start >= (uint32)v[CP_STABLE_MS]) {
            for(i=0;i<4;++i)ctl_saved_codes[i]=ctl_codes[i];
            ctl_profile=1;++ctl_revision;ctl_profile_config=ctl_config;
            if(!ctl_store()){ctl_learning_fail(3);return;}
            ctl_state=CONTROL_RUNNING;ctl_pi.fine=ctl_codes[3];ctl_pi.coarse=ctl_codes[2];
        }
    }
}
void control_service(void)
{
    uint32 now=ctl_ms;int32 counts;uint8 ready,saved;
    if(!ctl_initialized||ctl_state==CONTROL_PAUSED)return;
    saved=CyEnterCriticalSection();ready=ctl_ready;counts=ctl_average;ctl_ready=0;CyExitCriticalSection(saved);
    if(ready && ctl_channel==4) {
        int32 lp=(int32)(((int64)counts*1000000LL)/ADC_CF_2V5_COUNTS_PER_VOLT)-1000000;
        ctl_trace_v[ctl_trace_head]=lp;ctl_trace_ms[ctl_trace_head]=now;
        if(++ctl_trace_head>=CTL_TRACE_LEN){ctl_trace_head=0;ctl_trace_full=1;}
    }
    /* The trace keeps everything; the controller does not see readings taken
     * while the analog chain is still ringing from a telemetry burst. */
    if(ready && (int32)(ctl_quiet_until-now)>0)ready=0;
    if(ready) {
        /* sm_sample_cal_filt() already right-aligns the Delta-Sigma result.
         * ADC_CountsTo_uVolts() must not be used here: that generated helper
         * applies ADC_CF_2V5_DEC_DIV a second time (32 on this design).
         * Calibration forces CF_2V5, whose right-aligned scale is explicit. */
        int32 bank=(int32)(((int64)counts*1000000LL)/ADC_CF_2V5_COUNTS_PER_VOLT);
        ctl_med[ctl_med_i]=bank-1000000;ctl_med_i=(uint8)((ctl_med_i+1u)%CTL_MEDIAN_LEN);
        if(ctl_med_n<CTL_MEDIAN_LEN)++ctl_med_n;
    }
    if(ready && ctl_med_n<CTL_MEDIAN_LEN)ready=0;
    if(ready) {
        ctl_dc[ctl_channel]=ctl_median();
        ctl_valid[ctl_channel]=(uint8)control_measurement_valid(&ctl_config,ctl_dc[ctl_channel]);
        ctl_seen[ctl_channel]=1;ctl_stamp[ctl_channel]=now;
        if(ctl_channel==4) {
            /* "In band" is the controller's own verdict, so the flag (and its
             * I2C frame) only toggles when the loop freezes or wakes. */
            uint8 inside=ctl_valid[4] && ctl_pi.holding;
            uint8 old=ctl_band;
            if(!inside){ctl_enter_start=now;ctl_band=0;}
            else if(now-ctl_enter_start>=(uint32)ctl_config.value[CP_ENTER_MS])ctl_band=1;
            if(old!=ctl_band)ctl_emit(0x102,ctl_band,0);
            if((ctl_state==CONTROL_RUNNING || (ctl_state==CONTROL_LEARNING && ctl_phase==2)) && now-ctl_last_pi>=(uint32)ctl_config.value[CP_PERIOD_MS] &&
               ctl_seen[3] && now-ctl_stamp[3]<=ctl_scan_ms(now)*2u+(uint32)ctl_config.value[CP_SCAN_MS]) {
                control_pi_step(&ctl_pi,&ctl_config,now,ctl_dc[4],ctl_valid[4],ctl_dc[3],ctl_valid[3]);
                ctl_write(2,ctl_pi.coarse);ctl_write(3,ctl_pi.fine);ctl_last_pi=now;
            }
        }
        if(ctl_scan) {
            if(ctl_channel<4)ctl_select((uint8)(ctl_channel+1));
            else{ctl_scan=0;ctl_last_scan=now;}
        }
    }
    if(!ctl_scan && now-ctl_last_scan>=ctl_scan_ms(now)){ctl_scan=1;ctl_select(0);}
    if(ctl_state==CONTROL_LEARNING)ctl_learning(now);
    if(now-ctl_last_report>=(uint32)ctl_config.value[ctl_settled(now)?CP_REPORT_SETTLED_MS:CP_REPORT_MS]){ctl_report_start=ctl_ms;control_emit_snapshot(0);ctl_last_report=ctl_ms;}
}
uint8 control_command(uint8 cmd,uint8 p1,uint8 p2)
{
    uint8 i;
    if(ctl_state==CONTROL_PAUSED && !(cmd==0xD6 && p1==CONTROL_RESUME))return 0;
    if(cmd==0xD0){if(p1>=CP_COUNT)return 0;ctl_selected=p1;ctl_bytes=0;ctl_word=0;return 1;}
    if(cmd>=0xD1 && cmd<=0xD4){unsigned shift=8u*(cmd-0xD1);ctl_word=(ctl_word&~((uint32)255<<shift))|((uint32)p1<<shift);ctl_bytes|=(uint8)(1u<<(cmd-0xD1));return 1;}
    if(cmd==0xD5){if(ctl_bytes!=15)return 0;ctl_staged.value[ctl_selected]=(int32)ctl_word;ctl_bytes=0;return 1;}
    if(cmd==0xD7 && p1==0xFF){ctl_emit_trace();return 1;}
    if(cmd==0xD7){if(p1>=CP_COUNT)return 0;ctl_emit(p1,ctl_config.value[p1],0);return 1;}
    if(cmd!=0xD6)return 0;
    (void)p2;
    switch(p1) {
    case CONTROL_APPLY:
        if(ctl_state==CONTROL_LEARNING || !control_config_valid(&ctl_staged,PSOC_AMUX_SIGNAL_CHANNEL_COUNT,CAL_AMUX_HAS_CAP_CHANNEL))return 0;
        ctl_active=0;ADC_StopConvert();
        if(!control_config_profile_compatible(&ctl_staged,&ctl_profile_config))ctl_profile=0;
        ctl_config=ctl_staged;ctl_origin=2;ctl_report_count=0;
        psoc_hw_set_pga((uint8)ctl_config.value[CP_PGA]);psoc_hw_set_pgaout((uint8)ctl_config.value[CP_PGAOUT]);
        control_resume();return 1;
    case CONTROL_SAVE: if(ctl_state==CONTROL_LEARNING)return 0;return ctl_store();
    case CONTROL_DEFAULTS: control_config_defaults(&ctl_staged);return 1;
    case CONTROL_LEARN:
        if(ctl_state==CONTROL_LEARNING)return 0;
        ctl_learning_had_profile=ctl_profile;
        for(i=0;i<4;++i)ctl_learning_restore[i]=ctl_codes[i];
        ctl_state=CONTROL_LEARNING;ctl_phase=0;ctl_learning_refinements=0;
        ctl_learning_slow_step=0;ctl_learning_have_pos=ctl_learning_have_neg=0;
        ctl_learning_start=ctl_phase_start=ctl_ms;
        for(i=0;i<4;++i)ctl_write(i,i<2?0:(int16)ctl_config.value[i==2?CP_INITIAL2:CP_INITIAL3]);
        ctl_route_start();return 1;
    case CONTROL_PAUSE: control_prepare_capture();return 1;
    case CONTROL_RESUME: control_resume();return 1;
    case CONTROL_REPORT: ctl_report_count=0;control_emit_snapshot(0);return 1;
    default:return 0;
    }
}
