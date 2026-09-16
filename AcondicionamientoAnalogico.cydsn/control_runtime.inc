/* Included once by calibration.c: owns the analog mux through the same
 * exclusive-channel helper as diagnostics. The pure controller is host tested. */
#include "control_runtime.h"
#include "control_config.c"

static ControlConfig ctl_config, ctl_staged, ctl_profile_config;
static ControlPI ctl_pi;
static uint8 ctl_state, ctl_profile, ctl_origin, ctl_channel=4u, ctl_active;
static uint8 ctl_selected, ctl_bytes, ctl_band, ctl_scan, ctl_phase;
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
        ctl_image[2]==CONTROL_CONFIG_VERSION && ctl_image[3]==PSOC_HW_CLASS &&
        control_crc(ctl_image,254)==((uint16)ctl_image[254]|((uint16)ctl_image[255]<<8));
}
static uint8 ctl_store(void)
{
    uint8 slot=(uint8)(1u-ctl_nv_slot),row;
    uint16 crc,i;
    memset(ctl_image,0,sizeof ctl_image);
    ctl_image[0]=0x47;ctl_image[1]=0x43;ctl_image[2]=CONTROL_CONFIG_VERSION;ctl_image[3]=PSOC_HW_CLASS;
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
}
void control_emit_snapshot(uint8 capture)
{
    uint16 i;uint32 now=ctl_ms;
    ctl_emit(0x100,ctl_state,capture);ctl_emit(0x101,ctl_profile,capture);
    ctl_emit(0x102,ctl_band,capture);ctl_emit(0x103,ctl_origin,capture);
    ctl_emit(0x104,ctl_revision,capture);ctl_emit(0x105,(int32)now,capture);
    ctl_emit(0x106,CONTROL_CONFIG_VERSION,capture);
    ctl_emit(0x107,control_crc((const uint8 *)(__DATE__ " " __TIME__),sizeof(__DATE__ " " __TIME__)-1),capture);
    for(i=0;i<CP_COUNT;++i)ctl_emit(i,ctl_config.value[i],capture);
    for(i=0;i<4;++i)ctl_emit((uint16)(0x110+i),ctl_codes[i],capture);
    for(i=0;i<5;++i) {
        int32 margin=ctl_dc[i]-ctl_config.value[CP_VALID_LOW_UV];
        int32 upper=ctl_config.value[CP_VALID_HIGH_UV]-ctl_dc[i];
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
    ctl_channel=channel;ctl_sum=0;ctl_count=0;ctl_ready=0;
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
    ctl_pi.has_previous=0;ctl_pi.fraction=0;
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
        control_config_decode(&ctl_staged,ctl_image+8);
        if(control_config_valid(&ctl_staged,PSOC_AMUX_SIGNAL_CHANNEL_COUNT,CAL_AMUX_HAS_CAP_CHANNEL)) {
            ctl_config=ctl_staged;ctl_origin=1;ctl_generation=ctl_get32(ctl_image+4);
            pos=8+CONTROL_CONFIG_BYTES;ctl_profile=ctl_image[pos++]==1;
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
static uint8 ctl_all_healthy(uint32 now)
{
    uint8 i;
    for(i=0;i<5;++i)if(!ctl_seen[i]||!ctl_valid[i]||now-ctl_stamp[i]>(uint32)ctl_config.value[CP_SCAN_MS]*2u)return 0;
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
static void ctl_learning(uint32 now)
{
    const int32 *v=ctl_config.value;
    uint8 i;
    if(now-ctl_learning_start>(uint32)v[CP_LEARN_TIMEOUT_MS]) { ctl_learning_fail(1);return; }
    if(now-ctl_phase_start<(uint32)v[CP_SLOW_TAU_MS]*2u)return;
    if(ctl_phase==0 && ctl_seen[2] && ctl_valid[2]) {
        int a,b,ba=0,bb=0;int64 best=INT64_MAX,best_cost=INT64_MAX;
        int32 k0=(int32)(-(int64)v[CP_SLOW0_SLOPE_UV]*270/68);
        int32 k1=(int32)(-(int64)v[CP_SLOW1_SLOPE_UV]*2700/748);
        /* Bounded two-actuator least effort, integer version of Python search. */
        for(a=-255;a<=255;++a)for(b=-255;b<=255;++b) {
            int64 residual=magnitude((int64)ctl_dc[2]+a*k0+b*k1-v[CP_OPA_TARGET_UV]);
            int64 cost=(int64)a*a+(int64)b*b;
            int64 se=(int64)ctl_dc[0]+a*v[CP_SLOW0_SLOPE_UV];
            int64 bp=(int64)ctl_dc[1]+b*v[CP_SLOW1_SLOPE_UV];
            if(se<v[CP_VALID_LOW_UV]+v[CP_MARGIN_UV]||se>v[CP_VALID_HIGH_UV]-v[CP_MARGIN_UV]||
               bp<v[CP_VALID_LOW_UV]+v[CP_MARGIN_UV]||bp>v[CP_VALID_HIGH_UV]-v[CP_MARGIN_UV])continue;
            if((residual<=5000 && (best>5000||cost<best_cost)) || (best>5000 && residual<best))
                {best=residual;best_cost=cost;ba=a;bb=b;}
        }
        if(best==INT64_MAX){ctl_learning_fail(2);return;}
        ctl_write(0,(int16)ba);ctl_write(1,(int16)bb);
        ctl_phase=1;ctl_phase_start=now;ctl_stable_start=now;
        for(i=0;i<5;++i)ctl_anchor[i]=ctl_dc[i];
    } else if(ctl_phase==1) {
        if(!ctl_all_healthy(now)||magnitude(ctl_dc[2]-v[CP_OPA_TARGET_UV])>v[CP_LEARN_BAND_UV]) {
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
    if(ready) {
        /* sm_sample_cal_filt() already right-aligns the Delta-Sigma result.
         * ADC_CountsTo_uVolts() must not be used here: that generated helper
         * applies ADC_CF_2V5_DEC_DIV a second time (32 on this design).
         * Calibration forces CF_2V5, whose right-aligned scale is explicit. */
        int32 bank=(int32)(((int64)counts*1000000LL)/ADC_CF_2V5_COUNTS_PER_VOLT);
        ctl_dc[ctl_channel]=bank-1000000;
        ctl_valid[ctl_channel]=(ctl_dc[ctl_channel]>=ctl_config.value[CP_VALID_LOW_UV]&&ctl_dc[ctl_channel]<=ctl_config.value[CP_VALID_HIGH_UV]);
        ctl_seen[ctl_channel]=1;ctl_stamp[ctl_channel]=now;
        if(ctl_channel==4) {
            uint8 inside=ctl_valid[4] && magnitude(ctl_dc[4])<=ctl_config.value[CP_DEADBAND_UV];
            uint8 old=ctl_band;
            if(!inside){ctl_enter_start=now;ctl_band=0;}
            else if(now-ctl_enter_start>=(uint32)ctl_config.value[CP_ENTER_MS])ctl_band=1;
            if(old!=ctl_band)ctl_emit(0x102,ctl_band,0);
            if((ctl_state==CONTROL_RUNNING || (ctl_state==CONTROL_LEARNING && ctl_phase==1)) && now-ctl_last_pi>=(uint32)ctl_config.value[CP_PERIOD_MS] &&
               ctl_seen[3] && now-ctl_stamp[3]<=(uint32)ctl_config.value[CP_SCAN_MS]*2u) {
                control_pi_step(&ctl_pi,&ctl_config,now,ctl_dc[4],ctl_valid[4],ctl_dc[3],ctl_valid[3]);
                ctl_write(2,ctl_pi.coarse);ctl_write(3,ctl_pi.fine);ctl_last_pi=now;
            }
        }
        if(ctl_scan) {
            if(ctl_channel<4)ctl_select((uint8)(ctl_channel+1));
            else{ctl_scan=0;ctl_last_scan=now;}
        }
    }
    if(!ctl_scan && now-ctl_last_scan>=(uint32)ctl_config.value[CP_SCAN_MS]){ctl_scan=1;ctl_select(0);}
    if(ctl_state==CONTROL_LEARNING)ctl_learning(now);
    if(now-ctl_last_report>=(uint32)ctl_config.value[CP_REPORT_MS]){control_emit_snapshot(0);ctl_last_report=ctl_ms;}
}
uint8 control_command(uint8 cmd,uint8 p1,uint8 p2)
{
    uint8 i;
    if(ctl_state==CONTROL_PAUSED && !(cmd==0xD6 && p1==CONTROL_RESUME))return 0;
    if(cmd==0xD0){if(p1>=CP_COUNT)return 0;ctl_selected=p1;ctl_bytes=0;ctl_word=0;return 1;}
    if(cmd>=0xD1 && cmd<=0xD4){unsigned shift=8u*(cmd-0xD1);ctl_word=(ctl_word&~((uint32)255<<shift))|((uint32)p1<<shift);ctl_bytes|=(uint8)(1u<<(cmd-0xD1));return 1;}
    if(cmd==0xD5){if(ctl_bytes!=15)return 0;ctl_staged.value[ctl_selected]=(int32)ctl_word;ctl_bytes=0;return 1;}
    if(cmd==0xD7){if(p1>=CP_COUNT)return 0;ctl_emit(p1,ctl_config.value[p1],0);return 1;}
    if(cmd!=0xD6)return 0;
    (void)p2;
    switch(p1) {
    case CONTROL_APPLY:
        if(ctl_state==CONTROL_LEARNING || !control_config_valid(&ctl_staged,PSOC_AMUX_SIGNAL_CHANNEL_COUNT,CAL_AMUX_HAS_CAP_CHANNEL))return 0;
        ctl_active=0;ADC_StopConvert();
        if(!control_config_profile_compatible(&ctl_staged,&ctl_profile_config))ctl_profile=0;
        ctl_config=ctl_staged;ctl_origin=2;
        psoc_hw_set_pga((uint8)ctl_config.value[CP_PGA]);psoc_hw_set_pgaout((uint8)ctl_config.value[CP_PGAOUT]);
        control_resume();return 1;
    case CONTROL_SAVE: if(ctl_state==CONTROL_LEARNING)return 0;return ctl_store();
    case CONTROL_DEFAULTS: control_config_defaults(&ctl_staged);return 1;
    case CONTROL_LEARN:
        if(ctl_state==CONTROL_LEARNING)return 0;
        ctl_learning_had_profile=ctl_profile;
        for(i=0;i<4;++i)ctl_learning_restore[i]=ctl_codes[i];
        ctl_state=CONTROL_LEARNING;ctl_phase=0;ctl_learning_start=ctl_phase_start=ctl_ms;
        for(i=0;i<4;++i)ctl_write(i,i<2?0:(int16)ctl_config.value[i==2?CP_INITIAL2:CP_INITIAL3]);
        ctl_route_start();return 1;
    case CONTROL_PAUSE: control_prepare_capture();return 1;
    case CONTROL_RESUME: control_resume();return 1;
    case CONTROL_REPORT: control_emit_snapshot(0);return 1;
    default:return 0;
    }
}
