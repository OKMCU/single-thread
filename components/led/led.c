/******************************************************************************

 @file  hal_led.c

 @brief This file contains the interface to the HAL LED Service.

 Group: 
 Target Device: 

 ******************************************************************************
 

 ******************************************************************************
 Release Name: 
 Release Date: 
 *****************************************************************************/

/***************************************************************************************************
 * INCLUDES
 ***************************************************************************************************/

#include "st.h"
#include "hal.h"
#include "components/led/led.h"

/***************************************************************************************************
 * CONSTANTS
 ***************************************************************************************************/
#define LED_MAX                                 8
#define LED_TASK_EVT_UPDATE                     0
/***************************************************************************************************
 * MACROS
 ***************************************************************************************************/

/***************************************************************************************************
 * TYPEDEFS
 ***************************************************************************************************/

/* LED control structure */
typedef struct {
  st_uint8_t mode;       /* Operation mode */
  st_uint8_t left;       /* Blink cycles left */
  st_uint8_t onPct;      /* On cycle percentage */
  st_uint16_t time;      /* On/off cycle time (msec) */
  st_uint32_t next;      /* Time for next change */
} HalLedControl_t;

typedef struct
{
  HalLedControl_t HalLedControlTable[LED_MAX];
  st_uint8_t           sleepActive;
} HalLedStatus_t;

/***************************************************************************************************
 * GLOBAL VARIABLES
 ***************************************************************************************************/

static st_uint8_t HalLedState;              // LED state at last set/clr/blink update
static st_uint8_t HalSleepLedState;         // LED state at last set/clr/blink update
static st_uint8_t preBlinkState;            // Original State before going to blink mode
                                         // bit 0, 1, 2, 3 represent led 0, 1, 2, 3

#if ( LED_BLINK_EN > 0 )
static HalLedStatus_t HalLedStatusControl;
#endif

/***************************************************************************************************
 * LOCAL FUNCTIONS
 ***************************************************************************************************/

/***************************************************************************************************
 * @fn      led_on_off
 *
 * @brief   Turns specified LED ON or OFF
 *
 * @param   leds - LED bit mask
 *          mode - LED_ON,LED_OFF,
 *
 * @return  none
 ***************************************************************************************************/
static void led_on_off (st_uint8_t leds, st_uint8_t mode)
{
    if ( LED_0_PIN >= 0 )
    {
        if (leds & LED_0)
        {
            if (mode == LED_MODE_ON) hal_pin_write( LED_0_PIN, LED_ON_POLARITY ? HAL_PIN_HIGH : HAL_PIN_LOW );
            else                     hal_pin_write( LED_0_PIN, LED_ON_POLARITY ? HAL_PIN_LOW : HAL_PIN_HIGH );
        }
    }

    if ( LED_1_PIN >= 0 )
    {
        if (leds & LED_1)
        {
            if (mode == LED_MODE_ON) hal_pin_write( LED_1_PIN, LED_ON_POLARITY ? HAL_PIN_HIGH : HAL_PIN_LOW );
            else                     hal_pin_write( LED_1_PIN, LED_ON_POLARITY ? HAL_PIN_LOW : HAL_PIN_HIGH );
        }
    }


    if ( LED_2_PIN >= 0 )
    {
        if (leds & LED_2)
        {
            if (mode == LED_MODE_ON) hal_pin_write( LED_2_PIN, LED_ON_POLARITY ? HAL_PIN_HIGH : HAL_PIN_LOW );
            else                     hal_pin_write( LED_2_PIN, LED_ON_POLARITY ? HAL_PIN_LOW : HAL_PIN_HIGH );
        }
    }

    if ( LED_3_PIN >= 0 )
    {
        if (leds & LED_3)
        {
            if (mode == LED_MODE_ON) hal_pin_write( LED_3_PIN, LED_ON_POLARITY ? HAL_PIN_HIGH : HAL_PIN_LOW );
            else                     hal_pin_write( LED_3_PIN, LED_ON_POLARITY ? HAL_PIN_LOW : HAL_PIN_HIGH );
        }
    }


    if ( LED_4_PIN >= 0 )
    {
        if (leds & LED_4)
        {
            if (mode == LED_MODE_ON) hal_pin_write( LED_4_PIN, LED_ON_POLARITY ? HAL_PIN_HIGH : HAL_PIN_LOW );
            else                     hal_pin_write( LED_4_PIN, LED_ON_POLARITY ? HAL_PIN_LOW : HAL_PIN_HIGH );
        }
    }

    if ( LED_5_PIN >= 0 )
    {
        if (leds & LED_5)
        {
            if (mode == LED_MODE_ON) hal_pin_write( LED_5_PIN, LED_ON_POLARITY ? HAL_PIN_HIGH : HAL_PIN_LOW );
            else                     hal_pin_write( LED_5_PIN, LED_ON_POLARITY ? HAL_PIN_LOW : HAL_PIN_HIGH );
        }
    }

    if ( LED_6_PIN >= 0 )
    {
        if (leds & LED_6)
        {
            if (mode == LED_MODE_ON) hal_pin_write( LED_6_PIN, LED_ON_POLARITY ? HAL_PIN_HIGH : HAL_PIN_LOW );
            else                     hal_pin_write( LED_6_PIN, LED_ON_POLARITY ? HAL_PIN_LOW : HAL_PIN_HIGH );
        }
    }

    if ( LED_7_PIN >= 0 )
    {
        if (leds & LED_7)
        {
            if (mode == LED_MODE_ON) hal_pin_write( LED_7_PIN, LED_ON_POLARITY ? HAL_PIN_HIGH : HAL_PIN_LOW );
            else                     hal_pin_write( LED_7_PIN, LED_ON_POLARITY ? HAL_PIN_LOW : HAL_PIN_HIGH );
        }
    }
    
    /* Remember current state */
    if (mode)
    {
        HalLedState |= leds;
    }
    else
    {
        HalLedState &= (leds ^ 0xFF);
    }
}

/***************************************************************************************************
 * @fn      led_task_update
 *
 * @brief   Update leds to work with blink
 *
 * @param   none
 *
 * @return  none
 ***************************************************************************************************/
static void led_task_update (void)
{
  st_uint32_t time_sec;
  st_uint32_t time;
  HalLedControl_t *sts;
  st_uint16_t next;
  st_uint16_t wait;
  st_uint16_t time_ms;
  st_uint8_t led;
  st_uint8_t pct;
  st_uint8_t leds;
  

  next = 0;
  led  = LED_0;
  leds = LED_ALL;
  sts = HalLedStatusControl.HalLedControlTable;

  /* Check if sleep is active or not */
  if (!HalLedStatusControl.sleepActive)
  {
    while (leds)
    {
      if (leds & led)
      {
        if (sts->mode & LED_MODE_BLINK)
        {
          st_timer_get_time(&time_sec, &time_ms);
          time = time_sec*1000+time_ms;
          if (time >= sts->next)
          {
            if (sts->mode & LED_MODE_ON)
            {
              pct = 100 - sts->onPct;                   /* Percentage of cycle for off */
              sts->mode &= ~LED_MODE_ON;                /* Say it's not on */
              led_on_off (led, LED_MODE_OFF);           /* Turn it off */

              if ( !(sts->mode & LED_MODE_FLASH) )
              {
                sts->left--;                            // Not continuous, reduce count
              }
            }
            else if ( !(sts->left) && !(sts->mode & LED_MODE_FLASH) )
            {
              sts->mode ^= LED_MODE_BLINK;              // No more blinks
            }
            else
            {
              pct = sts->onPct;                         // Percentage of cycle for on
              sts->mode |= LED_MODE_ON;                 // Say it's on
              led_on_off( led, LED_MODE_ON );           // Turn it on
            }
            if (sts->mode & LED_MODE_BLINK)
            {
              wait = (((st_uint32_t)pct * (st_uint32_t)sts->time) / 100);
              sts->next = time + wait;
            }
            else
            {
              /* no more blink, no more wait */
              wait = 0;
              /* After blinking, set the LED back to the state before it blinks */
              led_set (led, ((preBlinkState & led)!=0)?LED_MODE_ON:LED_MODE_OFF);
              /* Clear the saved bit */
              preBlinkState &= (led ^ 0xFF);
            }
          }
          else
          {
            wait = sts->next - time;  /* Time left */
          }

          if (!next || ( wait && (wait < next) ))
          {
            next = wait;
          }
        }
        leds ^= led;
      }
      led <<= 1;
      sts++;
    }

    if (next)
    {
      st_timer_event_create( TASK_ID_LED, LED_TASK_EVT_UPDATE, next );/* Schedule event */
    }
  }
}

/***************************************************************************************************
 * FUNCTIONS - API
 ***************************************************************************************************/

/***************************************************************************************************
 * @fn      led_init
 *
 * @brief   Initialize LED Service
 *
 * @param   init - pointer to void that contains the initialized value
 *
 * @return  None
 ***************************************************************************************************/
void led_init (void)
{
    if ( LED_0_PIN >= 0 )
    {
        hal_pin_write( LED_0_PIN, LED_ON_POLARITY ? HAL_PIN_LOW : HAL_PIN_HIGH );
        hal_pin_mode( LED_0_PIN, HAL_PIN_MODE_OUTPUT );
    }
    
    if ( LED_1_PIN >= 0 )
    {
        hal_pin_write( LED_1_PIN, LED_ON_POLARITY ? HAL_PIN_LOW : HAL_PIN_HIGH );
        hal_pin_mode( LED_1_PIN, HAL_PIN_MODE_OUTPUT );
    }
    
    if ( LED_2_PIN >= 0 )
    {
        hal_pin_write( LED_2_PIN, LED_ON_POLARITY ? HAL_PIN_LOW : HAL_PIN_HIGH );
        hal_pin_mode( LED_2_PIN, HAL_PIN_MODE_OUTPUT );
    }

    if ( LED_3_PIN >= 0 )
    {
        hal_pin_write( LED_3_PIN, LED_ON_POLARITY ? HAL_PIN_LOW : HAL_PIN_HIGH );
        hal_pin_mode( LED_3_PIN, HAL_PIN_MODE_OUTPUT );
    }

    if ( LED_4_PIN >= 0 )
    {
        hal_pin_write( LED_4_PIN, LED_ON_POLARITY ? HAL_PIN_LOW : HAL_PIN_HIGH );
        hal_pin_mode( LED_4_PIN, HAL_PIN_MODE_OUTPUT );
    }

    if ( LED_5_PIN >= 0 )
    {
        hal_pin_write( LED_5_PIN, LED_ON_POLARITY ? HAL_PIN_LOW : HAL_PIN_HIGH );
        hal_pin_mode( LED_5_PIN, HAL_PIN_MODE_OUTPUT );
    }

    if ( LED_6_PIN >= 0 )
    {
        hal_pin_write( LED_6_PIN, LED_ON_POLARITY ? HAL_PIN_LOW : HAL_PIN_HIGH );
        hal_pin_mode( LED_6_PIN, HAL_PIN_MODE_OUTPUT );
    }

    if ( LED_7_PIN >= 0 )
    {
        hal_pin_write( LED_7_PIN, LED_ON_POLARITY ? HAL_PIN_LOW : HAL_PIN_HIGH );
        hal_pin_mode( LED_7_PIN, HAL_PIN_MODE_OUTPUT );
    }


#if ( LED_BLINK_EN > 0 )
  HalLedStatusControl.sleepActive = FALSE;          // Initialize sleepActive to FALSE.
#endif
}


void led_task( st_uint8_t event_id )
{
    ST_ASSERT( event_id == LED_TASK_EVT_UPDATE );
    led_task_update();
}

/***************************************************************************************************
 * @fn      led_set
 *
 * @brief   Turn ON/OFF/TOGGLE given LEDs
 *
 * @param   led - bit mask value of leds to be turned ON/OFF/TOGGLE
 *          mode - BLINK, FLASH, TOGGLE, ON, OFF
 * @return  None
 ***************************************************************************************************/
st_uint8_t led_set ( st_uint8_t leds, st_uint8_t mode )
{
#if ( LED_BLINK_EN > 0 )
  st_uint8_t led;
  HalLedControl_t *sts;

  switch (mode)
  {
    case LED_MODE_BLINK:
      /* Default blink, 1 time, D% duty cycle */
      led_blink ( leds, 1, LED_DEFAULT_DUTY_CYCLE, LED_DEFAULT_FLASH_TIME );
      break;

    case LED_MODE_FLASH:
      /* Default flash, N times, D% duty cycle */
      led_blink ( leds, LED_DEFAULT_FLASH_COUNT, LED_DEFAULT_DUTY_CYCLE, LED_DEFAULT_FLASH_TIME );
      break;

    case LED_MODE_ON:
    case LED_MODE_OFF:
    case LED_MODE_TOGGLE:

      led = LED_0;
      leds &= LED_ALL;
      sts = HalLedStatusControl.HalLedControlTable;

      while (leds)
      {
        if (leds & led)
        {
          if (mode != LED_MODE_TOGGLE)
          {
            sts->mode = mode;  /* ON or OFF */
          }
          else
          {
            sts->mode ^= LED_MODE_ON;  /* Toggle */
          }
          led_on_off (led, sts->mode);
          leds ^= led;
        }
        led <<= 1;
        sts++;
      }
      break;

    default:
      break;
  }
#else
  led_on_off(leds, mode);
#endif

  return ( HalLedState );
}

/***************************************************************************************************
 * @fn      led_blink
 *
 * @brief   Blink the leds
 *
 * @param   leds       - bit mask value of leds to be blinked
 *          numBlinks  - number of blinks
 *          percent    - the percentage in each period where the led
 *                       will be on
 *          period     - length of each cycle in milliseconds
 *
 * @return  None
 ***************************************************************************************************/
void led_blink ( st_uint8_t leds, st_uint8_t numBlinks, st_uint8_t percent, st_uint16_t period)
{
#if ( LED_BLINK_EN > 0 )
  st_uint32_t time_sec;
  st_uint16_t time_ms;
  HalLedControl_t *sts;
  st_uint8_t led;

  if (leds && percent && period)
  {
    if (percent < 100)
    {
      led = LED_0;
      leds &= LED_ALL;
      sts = HalLedStatusControl.HalLedControlTable;

      while (leds)
      {
        if (leds & led)
        {
          /* Store the current state of the led before going to blinking if not already blinking */
          if(sts->mode < LED_MODE_BLINK )
          	preBlinkState |= (led & HalLedState);

          sts->mode  = LED_MODE_OFF;                        /* Stop previous blink */
          sts->time  = period;                              /* Time for one on/off cycle */
          sts->onPct = percent;                             /* % of cycle LED is on */
          sts->left  = numBlinks;                           /* Number of blink cycles */
          if (!numBlinks) sts->mode |= LED_MODE_FLASH;      /* Continuous */
          st_timer_get_time(&time_sec, &time_ms);
          sts->next = time_sec*1000+time_ms;                /* Start now */
          sts->mode |= LED_MODE_BLINK;                      /* Enable blinking */
          leds ^= led;
        }
        led <<= 1;
        sts++;
      }
      // Cancel any overlapping timer for blink events
      st_timer_event_delete( TASK_ID_LED, LED_TASK_EVT_UPDATE );
      st_task_event_set ( TASK_ID_LED, LED_TASK_EVT_UPDATE );
    }
    else
    {
      led_set (leds, LED_MODE_ON);                          /* >= 100%, turn on */
    }
  }
  else
  {
    led_set (leds, LED_MODE_OFF);                           /* No on time, turn off */
  }
#else
  percent = (leds & HalLedState) ? LED_MODE_OFF : LED_MODE_ON;
  led_on_off (leds, percent);                              /* Toggle */
#endif /* BLINK_LEDS && HAL_LED */
}

/***************************************************************************************************
 * @fn      led_get_state
 *
 * @brief   Dim LED2 - Dim (set level) of LED2
 *
 * @param   none
 *
 * @return  led state
 ***************************************************************************************************/
st_uint8_t led_get_state ()
{
  return HalLedState;
}

/***************************************************************************************************
 * @fn      led_enter_sleep
 *
 * @brief   Store current LEDs state before sleep
 *
 * @param   none
 *
 * @return  none
 ***************************************************************************************************/
void led_enter_sleep( void )
{
#if ( LED_BLINK_EN > 0 )
  /* Sleep ON */
  HalLedStatusControl.sleepActive = TRUE;
#endif /* BLINK_LEDS */
  /* Save the state of each led */
  HalSleepLedState = HalLedState;
  
  /* TURN OFF all LEDs to save power */
  led_on_off (LED_ALL, LED_MODE_OFF);

}

/***************************************************************************************************
 * @fn      led_exit_sleep
 *
 * @brief   Restore current LEDs state after sleep
 *
 * @param   none
 *
 * @return  none
 ***************************************************************************************************/
void led_exit_sleep( void )
{
  /* Load back the saved state */
  led_on_off(HalSleepLedState, LED_MODE_ON);

  /* Restart - This takes care BLINKING LEDS */
  st_task_event_set( TASK_ID_LED, LED_TASK_EVT_UPDATE );

#if ( LED_BLINK_EN > 0 )
  /* Sleep OFF */
  HalLedStatusControl.sleepActive = FALSE;
#endif /* BLINK_LEDS */
}

/***************************************************************************************************
***************************************************************************************************/



