/*******************************************************************************
 * Copyright (c) 2021-2022, PEOS Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 * 
 * Change Logs:
 * Date         Author       Notes
 * 2021-10-30   Wentao SUN   first version
 *
 ******************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "stm32l0xx_ll_bus.h"
#include "stm32l0xx_ll_gpio.h"
#include "stm32l0xx_ll_usart.h"
#include "hal_uart.h"
#include "components/fifo/fifo.h"
#include "components/utilities/rbuf.h"

#ifdef OS_USING_HAL_UART
/* Exported variables --------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define UART0_RX_CACHE_SIZE         8
#define UART0_TX_CACHE_SIZE         8
#define UART1_RX_CACHE_SIZE         8
#define UART1_TX_CACHE_SIZE         8

#define TASK_EVT_UART0_RXD          0
#define TASK_EVT_UART1_RXD          1
#define TASK_EVT_UART0_PERR         2
#define TASK_EVT_UART1_PERR         3
#define TASK_EVT_UART0_OVF          4
#define TASK_EVT_UART1_OVF          5
#define TASK_EVT_UART0_IDLE         6
#define TASK_EVT_UART1_IDLE         7

#define TASK_EVT_UART0_TXD          0
#define TASK_EVT_UART1_TXD          1

#define CPU_APB1CLK                 (32000000uL)
#define CPU_APB2CLK                 (32000000uL)

/* Private typedef -----------------------------------------------------------*/
typedef struct {
    os_uint8_t *rx_cache;
    os_uint8_t *tx_cache;
    os_uint8_t rx_cache_size;
    os_uint8_t tx_cache_size;
} uart_cache_t;

typedef struct {
    void (*callback)( os_uint8_t event );
    os_uint8_t rx_head;
    os_uint8_t rx_tail;
    os_uint8_t tx_head;
    os_uint8_t tx_tail;
} uart_ctrl_t;

typedef struct {
    os_uint8_t rxd;
    os_uint8_t txd;
    os_uint8_t ovf;
    os_uint8_t perr;
    os_uint8_t idle;
} uart_event_t;

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static os_uint8_t uart0_rx_cache[UART0_RX_CACHE_SIZE];
static os_uint8_t uart0_tx_cache[UART0_TX_CACHE_SIZE];
static os_uint8_t task_id_rxd;
static os_uint8_t task_id_txd;

static uart_cache_t const uart_cache[HAL_UART_PORT_MAX] = {
    { 
        .rx_cache = uart0_rx_cache, 
        .tx_cache = uart0_tx_cache, 
        .rx_cache_size = sizeof(uart0_rx_cache), 
        .tx_cache_size = sizeof(uart0_tx_cache),
    },
};

static USART_TypeDef* const USARTx[HAL_UART_PORT_MAX] = {
    USART2
};

static os_uint32_t const PeriphClk[HAL_UART_PORT_MAX] = {
    CPU_APB1CLK
};

static uart_event_t const uart_event[HAL_UART_PORT_MAX] = {
    {
        .rxd = TASK_EVT_UART0_RXD,
        .txd = TASK_EVT_UART0_TXD,
        .ovf = TASK_EVT_UART0_OVF,
        .perr = TASK_EVT_UART0_PERR,
        .idle = TASK_EVT_UART0_IDLE,
    },
};

static uart_ctrl_t uart_ctrl[HAL_UART_PORT_MAX] = { 0 };

/* Private function prototypes -----------------------------------------------*/
static void hal_uart_isr( os_uint8_t port );

extern void USART2_IRQHandler( void );

/* Exported function implementations -----------------------------------------*/
void hal_uart_rxd_init( os_uint8_t task_id )
{
    task_id_rxd = task_id;
}

void hal_uart_rxd_task( os_int8_t event_id )
{
    switch ( event_id )
    {
        case TASK_EVT_UART0_RXD:
            if( uart_ctrl[HAL_UART_PORT_0].callback )
                uart_ctrl[HAL_UART_PORT_0].callback( HAL_UART_EVENT_RXD );
        break;

        case TASK_EVT_UART1_RXD:
        break;

        case TASK_EVT_UART0_PERR:
            if( uart_ctrl[HAL_UART_PORT_0].callback )
                uart_ctrl[HAL_UART_PORT_0].callback( HAL_UART_EVENT_PERR );
        break;

        case TASK_EVT_UART1_PERR:
        break;

        case TASK_EVT_UART0_OVF:
            if( uart_ctrl[HAL_UART_PORT_0].callback )
                uart_ctrl[HAL_UART_PORT_0].callback( HAL_UART_EVENT_OVF );
        break;

        case TASK_EVT_UART1_OVF:
        break;

        case TASK_EVT_UART0_IDLE:
            if( uart_ctrl[HAL_UART_PORT_0].callback )
                uart_ctrl[HAL_UART_PORT_0].callback( HAL_UART_EVENT_IDLE );
        break;

        case TASK_EVT_UART1_IDLE:
        break;
    }
}

void hal_uart_txd_init( os_uint8_t task_id )
{
    task_id_txd = task_id;
}

void hal_uart_txd_task( os_int8_t event_id )
{
    switch ( event_id )
    {
        case TASK_EVT_UART0_TXD:
            if( uart_ctrl[HAL_UART_PORT_0].callback )
                uart_ctrl[HAL_UART_PORT_0].callback( HAL_UART_EVENT_TXD );
        break;

        case TASK_EVT_UART1_TXD:
        break;
    }
}


/**
  * @brief  Function brief
  * @param  param1
  * @param  param2
  * @note   None
  * @retval None
  */
void hal_uart_open( os_uint8_t port, const hal_uart_config_t *cfg )
{
    OS_ASSERT( port < HAL_UART_PORT_MAX );
    OS_ASSERT( cfg != NULL );
    OS_ASSERT( !LL_USART_IsEnabled(USARTx[port]) );
    
    // reset peripherals firstly
    switch ( port )
    {
        case HAL_UART_PORT_0:
            LL_APB1_GRP1_EnableClock( LL_APB1_GRP1_PERIPH_USART2 );
            LL_APB1_GRP1_ForceReset( LL_APB1_GRP1_PERIPH_USART2 );
            LL_APB1_GRP1_ReleaseReset( LL_APB1_GRP1_PERIPH_USART2 );
            LL_GPIO_SetPinPull( GPIOA, LL_GPIO_PIN_2, LL_GPIO_PULL_UP );
            LL_GPIO_SetPinPull( GPIOA, LL_GPIO_PIN_15, LL_GPIO_PULL_UP );
            LL_GPIO_SetPinMode( GPIOA, LL_GPIO_PIN_2, LL_GPIO_MODE_ALTERNATE );     // PA2:  USART2_TX
            LL_GPIO_SetPinMode( GPIOA, LL_GPIO_PIN_15, LL_GPIO_MODE_ALTERNATE );    // PA15: USART2_RX
            LL_GPIO_SetAFPin_0_7( GPIOA, LL_GPIO_PIN_2, LL_GPIO_AF_4 );             // PA2:  USART2_TX
            LL_GPIO_SetAFPin_8_15( GPIOA, LL_GPIO_PIN_15, LL_GPIO_AF_4 );           // PA15: USART2_RX
            NVIC_EnableIRQ( USART2_IRQn );
        break;
    }

    // set baud rate
    LL_USART_SetBaudRate( USARTx[port],
                          PeriphClk[port],
                          LL_USART_OVERSAMPLING_16, 
                          cfg->baud_rate );

    // set data bits
    switch ( cfg->data_bits )
    {
        //case HAL_UART_DATA_BITS_7:
        //    LL_USART_SetDataWidth( USARTx[port], LL_USART_DATAWIDTH_7B );
        //break;
        //case HAL_UART_DATA_BITS_8:
        //    LL_USART_SetDataWidth( USARTx[port], LL_USART_DATAWIDTH_8B );
        //break;
        //case HAL_UART_DATA_BITS_9:
        //    LL_USART_SetDataWidth( USARTx[port], LL_USART_DATAWIDTH_9B );
        //break;
        default:
            LL_USART_SetDataWidth( USARTx[port], LL_USART_DATAWIDTH_8B );
        break;
    }

    // set stop bits
    switch ( cfg->stop_bits )
    {
        case HAL_UART_STOP_BITS_1:
            LL_USART_SetStopBitsLength( USARTx[port], LL_USART_STOPBITS_1 );
        break;

        case HAL_UART_STOP_BITS_2:
            LL_USART_SetStopBitsLength( USARTx[port], LL_USART_STOPBITS_2 );
        break;

        default:
            LL_USART_SetStopBitsLength( USARTx[port], LL_USART_STOPBITS_1 );
        break;
    }

    // set parity
    switch ( cfg->parity )
    {
        case HAL_UART_PARITY_EVEN:
            LL_USART_SetParity( USARTx[port], LL_USART_PARITY_EVEN );
            LL_USART_EnableIT_PE( USARTx[port] );
        break;
        case HAL_UART_PARITY_ODD:
            LL_USART_SetParity( USARTx[port], LL_USART_PARITY_ODD );
            LL_USART_EnableIT_PE( USARTx[port] );
        break;
        default:
            LL_USART_SetParity( USARTx[port], LL_USART_PARITY_NONE );
        break;
    }

    // set bit order
    if( cfg->bit_order == HAL_UART_BIT_ORDER_LSB )
        LL_USART_SetTransferBitOrder( USARTx[port], LL_USART_BITORDER_LSBFIRST );
    else
        LL_USART_SetTransferBitOrder( USARTx[port], LL_USART_BITORDER_MSBFIRST );

    // set data invert
    if( cfg->invert == HAL_UART_NRZ_NORMAL )
        LL_USART_SetBinaryDataLogic( USARTx[port] , LL_USART_BINARY_LOGIC_POSITIVE );
    else
        LL_USART_SetBinaryDataLogic( USARTx[port] , LL_USART_BINARY_LOGIC_NEGATIVE );

    // init uart control body info
    os_memset( &uart_ctrl[port], 0, sizeof(uart_ctrl_t) );
    uart_ctrl[port].callback = cfg->callback;
    
    LL_USART_EnableDirectionRx( USARTx[port] );
    LL_USART_EnableDirectionTx( USARTx[port] );
    LL_USART_EnableIT_RXNE( USARTx[port] );
    LL_USART_EnableIT_IDLE( USARTx[port] );
    LL_USART_Enable( USARTx[port] );
}

/**
  * @brief  Function brief
  * @param  param1
  * @param  param2
  * @note   None
  * @retval None
  */
void hal_uart_putc( os_uint8_t port, os_uint8_t byte )
{
    OS_ASSERT( port < HAL_UART_PORT_MAX );
    OS_ASSERT( LL_USART_IsEnabled(USARTx[port]) );
    
    while( RING_BUF_FULL(uart_ctrl[port].tx_head, 
                         uart_ctrl[port].tx_tail, 
                         uart_cache[port].tx_cache_size) );

    if( RING_BUF_EMPTY(uart_ctrl[port].tx_head, uart_ctrl[port].tx_tail) &&
        LL_USART_IsActiveFlag_TXE(USARTx[port]) )
    {
        LL_USART_TransmitData8( USARTx[port], byte );
        LL_USART_EnableIT_TXE( USARTx[port] );
    }
    else
    {
        LL_USART_DisableIT_TXE( USARTx[port] );
        RING_BUF_PUT( byte,
                      uart_ctrl[port].tx_head, 
                      uart_cache[port].tx_cache, 
                      uart_cache[port].tx_cache_size );
        LL_USART_EnableIT_TXE( USARTx[port] );
    }
}

/**
  * @brief  Function brief
  * @param  param1
  * @param  param2
  * @note   None
  * @retval None
  */
os_uint8_t hal_uart_getc( os_uint8_t port )
{
    os_uint8_t byte;
    
    OS_ASSERT( port < HAL_UART_PORT_MAX );
    OS_ASSERT( LL_USART_IsEnabled(USARTx[port]) );

    while( RING_BUF_EMPTY(uart_ctrl[port].rx_head, 
                          uart_ctrl[port].rx_tail) );

    LL_USART_DisableIT_RXNE( USARTx[port] );
    RING_BUF_GET( &byte, 
                  uart_ctrl[port].rx_tail, 
                  uart_cache[port].rx_cache, 
                  uart_cache[port].rx_cache_size );
    LL_USART_EnableIT_RXNE( USARTx[port] );

    return byte;
}

os_uint8_t hal_uart_tx_buf_free( os_uint8_t port )
{
    OS_ASSERT( port < HAL_UART_PORT_MAX );
    return RING_BUF_FREE_SIZE( uart_ctrl[port].tx_head, 
                               uart_ctrl[port].tx_tail, 
                               uart_cache[port].tx_cache_size );
}

os_uint8_t hal_uart_rx_buf_used( os_uint8_t port )
{
    OS_ASSERT( port < HAL_UART_PORT_MAX );
    return RING_BUF_USED_SIZE( uart_ctrl[port].rx_head, 
                               uart_ctrl[port].rx_tail, 
                               uart_cache[port].rx_cache_size );
}

/**
  * @brief  Function brief
  * @param  param1
  * @param  param2
  * @note   None
  * @retval None
  */
void hal_uart_close( os_uint8_t port )
{
    OS_ASSERT( port < HAL_UART_PORT_MAX );
    OS_ASSERT( LL_USART_IsEnabled(USARTx[port]) );

    LL_USART_DisableDirectionTx( USARTx[port] );
    LL_USART_DisableDirectionRx( USARTx[port] );
    LL_USART_DisableIT_RXNE( USARTx[port] );
    LL_USART_DisableIT_IDLE( USARTx[port] );
    LL_USART_Disable( USARTx[port] );

    switch ( port )
    {
        case HAL_UART_PORT_0:
            NVIC_DisableIRQ( USART2_IRQn );
            LL_GPIO_SetPinMode( GPIOA, LL_GPIO_PIN_2, LL_GPIO_MODE_ANALOG ); // PA2: USART2_TX
            LL_GPIO_SetPinMode( GPIOA, LL_GPIO_PIN_15, LL_GPIO_MODE_ANALOG ); // PA15: USART2_RX
            LL_GPIO_SetPinPull( GPIOA, LL_GPIO_PIN_2, LL_GPIO_PULL_NO );
            LL_GPIO_SetPinPull( GPIOA, LL_GPIO_PIN_15, LL_GPIO_PULL_NO );
            LL_APB2_GRP1_DisableClock( LL_APB1_GRP1_PERIPH_USART2 );
        break;
    }
}

/* Private function implementations ------------------------------------------*/
/**
  * @brief  Function brief
  * @param  param1
  * @param  param2
  * @note   None
  * @retval None
  */
static void hal_uart_isr( os_uint8_t port )
{
    os_uint8_t byte;

    OS_ASSERT( port < HAL_UART_PORT_MAX );
    
    if( LL_USART_IsActiveFlag_RXNE(USARTx[port]) )
    {
        byte = LL_USART_ReceiveData8( USARTx[port] );
        if( RING_BUF_FULL(uart_ctrl[port].rx_head, 
                          uart_ctrl[port].rx_tail, 
                          uart_cache[port].rx_cache_size) )
        {
            os_task_set_event( task_id_rxd, uart_event[port].ovf );
        }
        else
        {
            RING_BUF_PUT( byte, 
                          uart_ctrl[port].rx_head, 
                          uart_cache[port].rx_cache, 
                          uart_cache[port].rx_cache_size );
            os_task_set_event( task_id_rxd, uart_event[port].rxd );
        }
        return;
    }

    if( LL_USART_IsActiveFlag_PE(USARTx[port]) )
    {
        os_task_set_event( task_id_rxd, uart_event[port].perr );
        LL_USART_ClearFlag_PE( USARTx[port] );
        return;
    }

    if( LL_USART_IsActiveFlag_IDLE( USARTx[port]) )
    {
        os_task_set_event( task_id_rxd, uart_event[port].idle );
        LL_USART_ClearFlag_IDLE( USARTx[port] );
        return;
    }
    
    if( LL_USART_IsActiveFlag_TXE(USARTx[port]) )
    {
        if( RING_BUF_EMPTY(uart_ctrl[port].tx_head, uart_ctrl[port].tx_tail) )
        {
            LL_USART_DisableIT_TXE( USARTx[port] );
        }
        else
        {
            RING_BUF_GET( &byte, 
                          uart_ctrl[port].tx_tail, 
                          uart_cache[port].tx_cache, 
                          uart_cache[port].tx_cache_size );
            LL_USART_TransmitData8( USARTx[port], byte );
            os_task_set_event( task_id_txd, uart_event[port].txd );
        }
        return;
    }

    
}

void USART2_IRQHandler( void )
{
    hal_uart_isr( HAL_UART_PORT_0 );
}
#endif //OS_USING_HAL_UART
/****** (C) COPYRIGHT 2021 PEOS Development Team. *****END OF FILE****/
