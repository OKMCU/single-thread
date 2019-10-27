/******************************************************************************

 @file  demo.c

 @brief 

 Group: 
 Target Device: 

 ******************************************************************************
 

 ******************************************************************************
 Release Name: 
 Release Date: 
 *****************************************************************************/

/**************************************************************************************************
 * INCLUDES
 **************************************************************************************************/
#include "st.h"
#include "hal.h"
#include "components/led/led.h"
#include "demo.h"


/**************************************************************************************************
 * TYPE DEFINES
 **************************************************************************************************/

 /**************************************************************************************************
 * LOCAL API DECLARATION
 **************************************************************************************************/

/**************************************************************************************************
 * CONSTANTS
 **************************************************************************************************/

/**************************************************************************************************
 * GLOBAL VARIABLES
 **************************************************************************************************/
void demo_init( void )
{
    st_task_event_set( TASK_ID_DEMO, DEMO_TASK_EVT_LED_BLINK_FAST );
    //st_timer_event_create( TASK_ID_DEMO, DEMO_TASK_EVT_LED_BLINK_FAST, 10000 );
    st_timer_event_create( TASK_ID_DEMO, DEMO_TASK_EVT_LED_BLINK_SLOW, 10000 );
}

void demo_task( uint8_t event_id )
{
    switch( event_id )
    {
        case DEMO_TASK_EVT_LED_BLINK_FAST:
            led_blink( LED_ALL, 0, 50, 300 );
        break;

        case DEMO_TASK_EVT_LED_BLINK_SLOW:
            led_blink( LED_ALL, 0, 50, 1000 );
        break;

        default:
            ST_ASSERT_FORCED();
        break;
    }
}

/**************************************************************************************************
**************************************************************************************************/
