#include "control_config.h"
#include <string.h>

static int32_t bounded(int32_t x, int32_t lo, int32_t hi)
{ return x < lo ? lo : (x > hi ? hi : x); }
static int64_t magnitude(int64_t x) { return x < 0 ? -x : x; }
void control_config_defaults(ControlConfig *c)
{
    static const int32_t defaults[CP_COUNT] = {
        2, 5, 54, 1, 5000, 30000, 1, 4, 100000, 1000,
        30, 80, 40000, 1500000, 1, 7000, -1400000,
        4296, 1750, 43700, -270000, 500000, 600000,
        5000, 1000, 4, 208, -2400000, 2000000, 250000,
        0, 0, 10000, 10000, 255, 255
    };
    memcpy(c->value, defaults, sizeof defaults);
}
int control_config_valid(const ControlConfig *c, unsigned channels, unsigned has_cap)
{
    unsigned i;
    static const int32_t low[CP_COUNT] = {
        0,0,1,0,20,1000,0,1,1000,0,1,1,100,10000,1,
        -10000000,-10000000,-1000000,-1000000,1000,-2000000,1000,10000,
        1000,100,0,128,-5000000,1000,0,-255,-255,100,1000,1,1
    };
    static const int32_t high[CP_COUNT] = {
        8,8,4096,1,60000,600000,100,100,1000000,60000,255,255,600000,
        2400000,8,10000000,10000000,1000000,1000000,120000,2000000,
        1000000,3600000,600000,60000,4,26040,-1000,5000000,2000000,
        255,255,100000,120000,255,255
    };
    for (i=0; i<CP_COUNT; ++i)
        if(c->value[i]<low[i] || c->value[i]>high[i]) return 0;
    if ((unsigned)c->value[CP_CAPTURE_CHANNEL]>=channels ||
        (c->value[CP_CAPACITOR] && !has_cap)) return 0;
    if (magnitude(c->value[CP_FINE_SLOPE_UV])<100 ||
        magnitude(c->value[CP_COARSE_SLOPE_UV])<100 ||
        magnitude(c->value[CP_SLOW0_SLOPE_UV])<100 ||
        magnitude(c->value[CP_SLOW1_SLOPE_UV])<100) return 0;
    if (c->value[CP_LEARN_TIMEOUT_MS] < 4*c->value[CP_SLOW_TAU_MS]+c->value[CP_STABLE_MS]) return 0;
    if (c->value[CP_MARGIN_UV]*2 >= c->value[CP_VALID_HIGH_UV]-c->value[CP_VALID_LOW_UV]) return 0;
    return 1;
}
int control_config_profile_compatible(const ControlConfig *a, const ControlConfig *b)
{
    static const unsigned keys[] = {CP_PGA,CP_PGAOUT,CP_CAPACITOR,
        CP_FINE_SLOPE_UV,CP_COARSE_SLOPE_UV,CP_SLOW0_SLOPE_UV,
        CP_SLOW1_SLOPE_UV,CP_OPA_TARGET_UV,CP_INITIAL2,CP_INITIAL3};
    unsigned i;
    for(i=0;i<sizeof keys/sizeof keys[0];++i)
        if(a->value[keys[i]]!=b->value[keys[i]]) return 0;
    return 1;
}
void control_config_encode(const ControlConfig *c, uint8_t *out)
{
    unsigned i,j;
    for(i=0;i<CP_COUNT;++i) for(j=0;j<4;++j)
        out[4*i+j]=(uint8_t)((uint32_t)c->value[i]>>(8*j));
}
void control_config_decode(ControlConfig *c, const uint8_t *in)
{
    unsigned i;
    for(i=0;i<CP_COUNT;++i) c->value[i]=(int32_t)((uint32_t)in[4*i] |
        ((uint32_t)in[4*i+1]<<8) | ((uint32_t)in[4*i+2]<<16) | ((uint32_t)in[4*i+3]<<24));
}
uint16_t control_crc(const uint8_t *data, unsigned n)
{
    uint16_t crc=0xffffu; unsigned i;
    while(n--) { crc^=(uint16_t)*data++<<8;
        for(i=0;i<8;++i) crc=(uint16_t)((crc<<1)^((crc&0x8000u)?0x1021u:0)); }
    return crc;
}
void control_pi_step(ControlPI *p,const ControlConfig *c,uint32_t now,
                     int32_t lp,int valid,int32_t sum,int sum_valid)
{
    const int32_t *v=c->value;
    int32_t step=0, e=-lp;
    uint32_t dt=p->has_previous ? now-p->previous_ms : (uint32_t)v[CP_PERIOD_MS];
    int64_t increment, denom;
    /* Coarse changes affect SUM with opposite sign to their LP effect. Both
     * rescue branches share a physical cooldown; never pile up coarse steps. */
    if(!sum_valid || magnitude(sum)>v[CP_SUM_BAND_UV]) {
        p->has_previous=0; p->fraction=0;
        if(!p->has_rescue || now-p->rescue_ms >= (uint32_t)v[CP_RESCUE_MS]) {
            step=((sum>0)==(v[CP_COARSE_SLOPE_UV]>0)) ? v[CP_COARSE_STEP] : -v[CP_COARSE_STEP];
            p->coarse=(int16_t)bounded(p->coarse+step,-v[CP_COARSE_LIMIT],v[CP_COARSE_LIMIT]);
            p->rescue_ms=now; p->has_rescue=1;
        }
        return;
    }
    if(!valid) {
        p->has_previous=0; p->fraction=0;
        if(!p->has_rescue || now-p->rescue_ms >= (uint32_t)v[CP_RESCUE_MS]) {
            step=((e>0)==(v[CP_FINE_SLOPE_UV]>0)) ? v[CP_RESCUE_STEP] : -v[CP_RESCUE_STEP];
            if((p->fine>=v[CP_FINE_LIMIT] && step>0)||(p->fine<=-v[CP_FINE_LIMIT] && step<0)) {
                step=((e>0)==(v[CP_COARSE_SLOPE_UV]>0)) ? 1 : -1;
                p->coarse=(int16_t)bounded(p->coarse+step,-v[CP_COARSE_LIMIT],v[CP_COARSE_LIMIT]);
            } else p->fine=(int16_t)bounded(p->fine+step,-v[CP_FINE_LIMIT],v[CP_FINE_LIMIT]);
            p->rescue_ms=now; p->has_rescue=1;
        }
        return;
    }
    /* True dead-zone: control only the excess beyond the allowed band.  Using
     * e=-lp here made a +101 mV sample request the full 101 mV correction,
     * which is needlessly aggressive for a plant with visible settling time. */
    if(lp>v[CP_DEADBAND_UV]) e=v[CP_DEADBAND_UV]-lp;
    else if(lp<-v[CP_DEADBAND_UV]) e=-v[CP_DEADBAND_UV]-lp;
    else { p->has_previous=0; p->fraction=0; return; }
    if(dt>(uint32_t)v[CP_PERIOD_MS]*2u) dt=(uint32_t)v[CP_PERIOD_MS];
    /* Q16 code accumulator. Kp remains tied to configured period, whereas
     * integration uses elapsed time. No ALPHA_MINIMA and no forced one-code. */
    denom=(int64_t)v[CP_TAU_MS]*v[CP_FINE_SLOPE_UV];
    increment=((int64_t)e+(p->has_previous?p->previous_error:e))*dt*32768/denom;
    increment+=(int64_t)(e-(p->has_previous?p->previous_error:0))*v[CP_PERIOD_MS]*65536/denom*v[CP_KP_NUM]/v[CP_KP_DEN];
    p->fraction+=increment;
    step=(int32_t)(p->fraction/65536);
    step=bounded(step,-v[CP_FINE_STEP],v[CP_FINE_STEP]);
    if(step!=0) {
        int32_t next=bounded(p->fine+step,-v[CP_FINE_LIMIT],v[CP_FINE_LIMIT]);
        p->fraction-=((int64_t)(next-p->fine))*65536;
        if(next==v[CP_FINE_LIMIT] || next==-v[CP_FINE_LIMIT] || magnitude(p->fraction)>65536) p->fraction=0;
        p->fine=(int16_t)next;
    }
    p->previous_error=e; p->previous_ms=now; p->has_previous=1;
}
