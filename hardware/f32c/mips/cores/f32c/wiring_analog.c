/* EMARD */

#include "Arduino.h"
#include "wiring_analog.h"

#ifdef __cplusplus
extern "C" {
#endif

static uint8_t analog_write_resolution_bits = 8;
/* old arduino uses 490 Hz */
/* new arduino uses 980 Hz */
static uint32_t analog_write_frequency = 980;
static uint32_t analog_write_phase = 0;

const struct pwm_enable_bitmask_s pwm_enable_bitmask[] = VARIANT_PWM_CHANNEL_ENABLE;

uint32_t analogRead(uint32_t ulPin)
{
  return 0;
}

/* todo: support analogWriteResolution(bits)
** default is 8 for old arduino,
** new arduino can use up to 12
*/
void analogWriteResolution(int res)
{
  analog_write_resolution_bits = res;
}

void analogWriteFrequency(int freq)
{
  analog_write_frequency = freq;
}

void analogWritePhase(int phase)
{
  analog_write_phase = phase;
}

/* setup the common parameters (why isn't it called at startup?)
*/
void analogOutputInit( void )
{
}

void analogWrite(uint32_t ulPin, uint32_t ulValue)
{
  int8_t pwm_channel;
  uint32_t *start, *stop;
  
  if(ulPin >= variant_pin_map_size)
    return;
    
  pwm_channel = variant_pin_map[ulPin].pwm;
  if(pwm_channel >= 0)
  {
    /* standard PWM frequency is 
    ** 490 Hz on old arduino
    ** 980 Hz in new arduino
    ** for better high freq coverage and compatibility with
    ** 12-bit analogWrite we need to recompile bitstream with 
    ** PRESCALER_BITS=10 and with TIMER_BITS=12
    ** to calculate increment:
    ** increment = (frequency_hz << (TIMER_BITS+PRESCALER_BITS) ) / TIMER_CLOCK_hz;
    */
    EMARD_TIMER[TC_INCREMENT] = (((uint64_t)analog_write_frequency) 
                                   << (TIMER_BITS+PRESCALER_BITS)) / TIMER_CLOCK;

    EMARD_TIMER[TC_CONTROL] &= pwm_enable_bitmask[pwm_channel].control_and;
    /* shift timer step value to match the set resolution */
    if(analog_write_resolution_bits < TIMER_BITS)
      ulValue <<= (TIMER_BITS-analog_write_resolution_bits);
    if(analog_write_resolution_bits > TIMER_BITS)
      ulValue >>= (analog_write_resolution_bits-analog_write_resolution_bits);
      
    start = &EMARD_TIMER[pwm_enable_bitmask[pwm_channel].ocp_start];
    stop  = &EMARD_TIMER[pwm_enable_bitmask[pwm_channel].ocp_stop];

    *start = ((1<<TIMER_BITS)-1) 
           &   analog_write_phase;
    *stop  = ((1<<TIMER_BITS)-1)
           & ( analog_write_phase
             + ( ulValue < (1<<TIMER_BITS) ? ulValue : (1<<TIMER_BITS) )
             );

    if( *start < *stop )
      EMARD_TIMER[TC_CONTROL] |= pwm_enable_bitmask[pwm_channel].control_and_or;

    EMARD_TIMER[TC_CONTROL] |= pwm_enable_bitmask[pwm_channel].control_or;
    EMARD_TIMER[TC_APPLY] = pwm_enable_bitmask[pwm_channel].apply;
  }
}
