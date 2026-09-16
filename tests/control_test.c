#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "../AcondicionamientoAnalogico.cydsn/control_config.h"
int main(void)
{
    ControlConfig c,d;ControlPI p={0};uint8_t bytes[CONTROL_CONFIG_BYTES];unsigned i;
    control_config_defaults(&c);
    assert(control_config_valid(&c,5,1));
    assert(!control_config_valid(&c,5,0));
    control_config_encode(&c,bytes);control_config_decode(&d,bytes);
    assert(!memcmp(&c,&d,sizeof c));
    { uint16_t crc=control_crc(bytes,sizeof bytes);bytes[7]^=1;assert(crc!=control_crc(bytes,sizeof bytes)); }
    d=c;d.value[CP_KP_DEN]=0;assert(!control_config_valid(&d,5,1));
    d=c;d.value[CP_PERIOD_MS]=20;assert(control_config_profile_compatible(&c,&d));
    d.value[CP_CAPACITOR]=0;assert(!control_config_profile_compatible(&c,&d));
    p.fine=12;p.coarse=75;
    control_pi_step(&p,&c,1000,50000,1,0,1);assert(p.fine==12&&p.coarse==75);
    /* Negative LP -> positive fine; no minimum-alpha acceleration at 50Hz. */
    memset(&p,0,sizeof p);c.value[CP_PERIOD_MS]=20;
    for(i=1;i<=50;++i)control_pi_step(&p,&c,i*20,-700000,1,0,1);
    assert(p.fine>=2&&p.fine<=3);
    /* Inside the band, and just at its boundary, the controller is inert. */
    { int16_t held=p.fine; control_pi_step(&p,&c,1020,100000,1,0,1); assert(p.fine==held); }
    /* Coarse rescue obeys the same 40s hold, including repeated rail samples. */
    memset(&p,0,sizeof p);p.coarse=75;
    control_pi_step(&p,&c,1000,-3000000,0,1980000,1);assert(p.coarse==74);
    for(i=2;i<40;++i)control_pi_step(&p,&c,i*1000,-3000000,0,1980000,1);
    assert(p.coarse==74);
    control_pi_step(&p,&c,41000,-3000000,0,1980000,1);assert(p.coarse==73);
    puts("control_test PASS: serialization, CRC, bounds, profile, PI time scaling, rescue hold");
    return 0;
}
