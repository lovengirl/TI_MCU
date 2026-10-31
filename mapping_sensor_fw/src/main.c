#include "platform.h"
#include "ti_msp_dl_config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Latest complete frame: inspect this and g_sensor from the CCS Expressions view. */
sensor_frame_t g_latest_frame;
volatile uint32_t g_frame_count;
static char line[96];static unsigned line_len;static bool line_bad;
static uint32_t last_rx_overruns;
static bool number(const char *text,unsigned max,unsigned *out){
    if(!text||!*text||*text<'0'||*text>'9')return false;
    char *end;unsigned long v=strtoul(text,&end,10);
    if(*end||v>max)return false;*out=(unsigned)v;return true;
}
static void reply(bool ok){console_puts(ok?"OK\r\n":"ERR\r\n");}
static void command(char *text){
    char *args[6];unsigned n=0;
    char *p=strtok(text," \t");while(p&&n<6){args[n++]=p;p=strtok(NULL," \t");}
    if(!n)return;if(p){reply(false);return;}
    if(!strcmp(args[0],"STATUS") && n==1){
        char b[200];snprintf(b,sizeof(b),"STATE=%u FAULT=%lu FRAME=%lu CFG=%lu RX_DROP=%lu TX_DROP=%lu\r\n",
          (unsigned)g_sensor.state,(unsigned long)g_sensor.fault,(unsigned long)g_frame_count,
          (unsigned long)g_sensor.config_version,(unsigned long)g_uart_rx_overruns,(unsigned long)g_uart_tx_dropped);console_puts(b);return;
    }
    if(!strcmp(args[0],"RESULT") && n==1){
        if(!g_frame_count){console_puts("NO_DATA\r\n");return;}
        char b[150];snprintf(b,sizeof(b),"FRAME=%lu CFG=%lu VALID=%05lX DETECT=%05lX SAT=%05lX\r\n",
          (unsigned long)g_latest_frame.sequence,(unsigned long)g_latest_frame.config_version,
          (unsigned long)g_latest_frame.valid,(unsigned long)g_latest_frame.detected,(unsigned long)g_latest_frame.saturated);console_puts(b);
        for(unsigned i=0;i<SENSOR_CHANNELS;i++){
          snprintf(b,sizeof(b),"CH=%u OFF=%u ON=%u SIG=%ld\r\n",i,g_latest_frame.off[i],g_latest_frame.on[i],(long)g_latest_frame.signal[i]);console_puts(b);
        }return;
    }
    if(!strcmp(args[0],"HELP")&&n==1){console_puts("STATUS RESULT SINGLE START STOP ESTOP CLEAR DAC ch code THRESH ch enter leave POLARITY 0|1 HELP\r\n");return;}
    if(!strcmp(args[0],"SAVE")&&n==1){console_puts("UNSUPPORTED: RAM settings only\r\n");return;}
    uint32_t key=__get_PRIMASK();__disable_irq();bool ok=false;
    if(!strcmp(args[0],"SINGLE")&&n==1)ok=sensor_start(&g_sensor,false);
    else if(!strcmp(args[0],"START")&&n==1)ok=sensor_start(&g_sensor,true);
    else if(!strcmp(args[0],"STOP")&&n==1){sensor_stop(&g_sensor,false);ok=true;}
    else if(!strcmp(args[0],"ESTOP")&&n==1){sensor_stop(&g_sensor,true);ok=true;}
    else if(!strcmp(args[0],"CLEAR")&&n==1)ok=sensor_clear_fault(&g_sensor);
    else {
      sensor_config_t c=g_sensor.has_pending?g_sensor.pending:g_sensor.config;
      unsigned ch,v,w;
      if(!strcmp(args[0],"DAC")&&n==3&&number(args[1],19,&ch)&&number(args[2],4095,&v)){
          c.dac[ch]=(uint16_t)v;ok=sensor_set_config(&g_sensor,&c);
      }else if(!strcmp(args[0],"THRESH")&&n==4&&number(args[1],19,&ch)&&number(args[2],32767,&v)&&number(args[3],32767,&w)){
          c.enter[ch]=(int16_t)v;c.leave[ch]=(int16_t)w;ok=sensor_set_config(&g_sensor,&c);
      }else if(!strcmp(args[0],"POLARITY")&&n==2&&number(args[1],1,&v)){
          c.polarity=(uint8_t)v;ok=sensor_set_config(&g_sensor,&c);
      }
    }
    __set_PRIMASK(key);reply(ok);
}
int main(void){
    platform_init();console_puts("MAPPING20 FW 0.1 READY / IDLE / RAM CONFIG\r\n");
    while(1){
      platform_poll();
      if(g_sensor.ready){
        sensor_process_ready(&g_sensor);g_latest_frame=*sensor_result(&g_sensor);g_frame_count++;
        uint32_t key=__get_PRIMASK();__disable_irq();sensor_release(&g_sensor);__set_PRIMASK(key);
      }
      char c;unsigned budget=16;
      if(last_rx_overruns!=g_uart_rx_overruns){last_rx_overruns=g_uart_rx_overruns;line_bad=true;}
      while(budget-- && console_get(&c)){
        if(c=='\r'||c=='\n'){
          if(line_bad)reply(false);else if(line_len){line[line_len]=0;command(line);}
          line_len=0;line_bad=false;
        }else if((unsigned char)c<32 || (unsigned char)c>126)line_bad=true;
        else if(line_len<sizeof(line)-1)line[line_len++]=c;
        else line_bad=true;
      }
      if(g_sensor.state==S_FAULT)DL_GPIO_setPins(GPIOB,BOARD_STATUS_PIN);
      else DL_GPIO_clearPins(GPIOB,BOARD_STATUS_PIN);
      /* Fault state is a controlled safe state. A stalled main loop is not. */
      static uint32_t last_feed;
      uint32_t now=platform_millis();
      if((uint32_t)(now-last_feed)>=100u){DL_WWDT_restart(WWDT0);last_feed=now;}
    }
}
