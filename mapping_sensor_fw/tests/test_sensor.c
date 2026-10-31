#include "sensor.h"
#include <string.h>
volatile uint32_t test_checks,test_fail_line,test_cases;
static sensor_t s;static bool tx_on,rx_on;static uint8_t selected;
static uint32_t violation;static uint16_t dac_code;
#define CHECK(x) do{test_checks++;if(!(x)){test_fail_line=__LINE__;return false;}}while(0)
static void tx(bool on){tx_on=on;}
static void rx(bool on){rx_on=on;}
static void select_ch(uint8_t c){if(tx_on||rx_on||c>=20)violation++;selected=c;}
static void dac(uint16_t v){if(tx_on)violation++;dac_code=v;}
static void wait(uint16_t v){(void)v;}
static void sample(uint16_t period,uint16_t *dest,uint8_t n){(void)period;(void)dest;if(n!=4)violation++;}
static void cancel(void){}
static void init(void){
 sensor_hal_t h={tx,rx,select_ch,dac,wait,sample,cancel};violation=0;tx_on=rx_on=false;sensor_init(&s,&h);
}
static bool frame(uint16_t off,uint16_t on){
 for(unsigned i=0;i<20;i++){
   CHECK(s.state==S_SELECT_WAIT);CHECK(selected==i);CHECK(!tx_on);CHECK(rx_on);
   sensor_wait_done(&s);CHECK(s.state==S_OFF_SAMPLE);CHECK(!tx_on);
   for(unsigned k=0;k<4;k++)s.samples[k]=off;
   sensor_sample_done(&s);CHECK(s.state==S_LIGHT_WAIT);CHECK(tx_on);
   sensor_wait_done(&s);CHECK(s.state==S_ON_SAMPLE);
   for(unsigned k=0;k<4;k++)s.samples[k]=on;
   sensor_sample_done(&s);
 }
 CHECK(s.ready);CHECK(!tx_on&&!rx_on&&dac_code==0);CHECK(violation==0);sensor_process_ready(&s);return true;
}
static bool test_basic(void){
 init();CHECK(s.state==S_IDLE&&!tx_on&&!rx_on);CHECK(sensor_start(&s,false));
 CHECK(!sensor_start(&s,false));CHECK(frame(100,900));
 const sensor_frame_t *f=sensor_result(&s);CHECK(f&&f->sequence==1&&f->valid==SENSOR_ALL);
 CHECK(f->signal[19]==800&&f->detected==0);sensor_release(&s);CHECK(!s.ready&&s.state==S_IDLE);return true;
}
static bool test_hysteresis(void){
 init();CHECK(sensor_start(&s,true));CHECK(frame(100,900));sensor_release(&s);
 CHECK(frame(100,900));CHECK(sensor_result(&s)->detected==SENSOR_ALL);sensor_release(&s);
 CHECK(frame(100,550));CHECK(sensor_result(&s)->detected==SENSOR_ALL);sensor_release(&s);
 CHECK(frame(100,450));CHECK(sensor_result(&s)->detected==SENSOR_ALL);sensor_release(&s);
 CHECK(frame(100,450));CHECK(sensor_result(&s)->detected==0);sensor_stop(&s,true);sensor_release(&s);
 CHECK(s.state==S_IDLE&&!tx_on&&!s.ready);return true;
}
static bool test_pending(void){
 init();CHECK(sensor_start(&s,true));sensor_config_t c=s.config;c.dac[0]=1000;
 CHECK(sensor_set_config(&s,&c));CHECK(s.config.dac[0]==512);CHECK(frame(100,900));
 CHECK(sensor_result(&s)->config_version==1);sensor_release(&s);
 CHECK(s.config_version==2&&dac_code==1000);CHECK(frame(100,900));
 c=s.config;c.dac[0]=1200;CHECK(sensor_set_config(&s,&c));
 CHECK(s.config_version==2);sensor_process_ready(&s);sensor_release(&s);CHECK(s.config_version==3&&dac_code==1200);return true;
}
static bool test_invalid(void){
 init();sensor_config_t c=s.config;c.dac[19]=4096;CHECK(!sensor_set_config(&s,&c));
 c=s.config;c.confirm=0;CHECK(!sensor_set_config(&s,&c));
 c=s.config;c.sample_us=1;CHECK(!sensor_set_config(&s,&c));
 c=s.config;c.leave[0]=c.enter[0];CHECK(!sensor_set_config(&s,&c));
 c=s.config;c.gain_q12[0]=0;CHECK(!sensor_set_config(&s,&c));CHECK(!sensor_config_valid(0));return true;
}
static bool test_stop(void){
 init();CHECK(sensor_start(&s,true));sensor_stop(&s,false);sensor_wait_done(&s);
 for(unsigned k=0;k<4;k++)s.samples[k]=100;sensor_sample_done(&s);sensor_wait_done(&s);
 for(unsigned k=0;k<4;k++)s.samples[k]=900;sensor_sample_done(&s);
 CHECK(s.state==S_IDLE&&!s.ready&&!tx_on&&!rx_on);return true;
}
static bool test_fault(void){
 init();CHECK(sensor_start(&s,true));sensor_wait_done(&s);
 for(unsigned k=0;k<4;k++)s.samples[k]=100;sensor_sample_done(&s);CHECK(tx_on);
 sensor_fail(&s,1);CHECK(s.state==S_FAULT&&!tx_on&&!rx_on&&s.fault==1);
 CHECK(!sensor_start(&s,true));CHECK(sensor_clear_fault(&s));CHECK(sensor_start(&s,false));CHECK(frame(100,900));return true;
}
static bool test_saturation(void){
 init();s.config.confirm=1;CHECK(sensor_start(&s,false));CHECK(frame(100,4095));
 CHECK(sensor_result(&s)->saturated==SENSOR_ALL&&sensor_result(&s)->valid==0&&sensor_result(&s)->detected==0);return true;
}
static bool test_polarity(void){
 init();s.config.polarity=0;s.config.confirm=1;s.config.offset[0]=100;s.config.gain_q12[0]=8192;
 CHECK(sensor_start(&s,false));CHECK(frame(900,100));
 CHECK(sensor_result(&s)->signal[0]==1400&&sensor_result(&s)->detected==SENSOR_ALL);return true;
}
static bool test_bad_event(void){init();sensor_wait_done(&s);CHECK(s.state==S_FAULT&&!tx_on);return true;}
static bool test_buffer(void){
 init();CHECK(sensor_start(&s,true));CHECK(frame(100,900));
 const sensor_frame_t *first=sensor_result(&s);uint32_t seq=first->sequence;
 CHECK(s.state==S_IDLE);CHECK(!sensor_start(&s,false));CHECK(first->sequence==seq);
 sensor_release(&s);CHECK(frame(100,800));CHECK(sensor_result(&s)!=first&&first->sequence==seq);return true;
}
static bool test_bad_adc(void){
 init();CHECK(sensor_start(&s,false));sensor_wait_done(&s);s.samples[0]=5000;
 sensor_sample_done(&s);CHECK(s.state==S_FAULT&&s.fault==4&&!tx_on);return true;
}
static bool test_estop_states(void){
 for(unsigned state=0;state<4;state++){
   init();CHECK(sensor_start(&s,true));
   if(state>=1)sensor_wait_done(&s);
   if(state>=2){for(unsigned k=0;k<4;k++)s.samples[k]=100;sensor_sample_done(&s);}
   if(state>=3)sensor_wait_done(&s);
   sensor_stop(&s,true);CHECK(s.state==S_IDLE&&!tx_on&&!rx_on&&!s.continuous&&!s.ready);
 }return true;
}
static bool test_repeated(void){
 init();CHECK(sensor_start(&s,true));
 for(unsigned f=0;f<100;f++){CHECK(frame(100,900));CHECK(sensor_result(&s)->sequence==f+1);sensor_release(&s);}
 sensor_stop(&s,true);CHECK(!tx_on);return true;
}
uint32_t tests_run(void){
 test_checks=test_fail_line=test_cases=0;
 bool (*cases[])(void)={test_basic,test_hysteresis,test_pending,test_invalid,test_stop,test_fault,test_saturation,test_polarity,test_bad_event,test_buffer,test_bad_adc,test_estop_states,test_repeated};
 for(unsigned i=0;i<sizeof(cases)/sizeof(cases[0]);i++){if(!cases[i]())return i+1;test_cases++;}return 0;
}
