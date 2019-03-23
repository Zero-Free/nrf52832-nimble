#ifndef RT_CONFIG_H__
#define RT_CONFIG_H__

/* Automatically generated file; DO NOT EDIT. */
/* RT-Thread Configuration */

/* RT-Thread Kernel */

#define RT_NAME_MAX 8
#define RT_ALIGN_SIZE 4
#define RT_THREAD_PRIORITY_32
#define RT_THREAD_PRIORITY_MAX 32
#define RT_TICK_PER_SECOND 100
#define RT_USING_OVERFLOW_CHECK
#define RT_USING_HOOK
#define RT_USING_IDLE_HOOK
#define RT_IDEL_HOOK_LIST_SIZE 4
#define IDLE_THREAD_STACK_SIZE 256
#define RT_DEBUG

/* Inter-Thread communication */

#define RT_USING_SEMAPHORE
#define RT_USING_MUTEX
#define RT_USING_EVENT
#define RT_USING_MAILBOX
#define RT_USING_MESSAGEQUEUE

/* Memory Management */

#define RT_USING_MEMPOOL
#define RT_USING_SMALL_MEM
#define RT_USING_HEAP

/* Kernel Device Object */

#define RT_USING_DEVICE
#define RT_USING_CONSOLE
#define RT_CONSOLEBUF_SIZE 128
#define RT_CONSOLE_DEVICE_NAME "uart0"
#define RT_VER_NUM 0x40000

/* RT-Thread Components */

#define RT_USING_COMPONENTS_INIT
#define RT_USING_USER_MAIN
#define RT_MAIN_THREAD_STACK_SIZE 2048
#define RT_MAIN_THREAD_PRIORITY 10

/* C++ features */


/* Command shell */

#define RT_USING_FINSH
#define FINSH_THREAD_NAME "tshell"
#define FINSH_USING_HISTORY
#define FINSH_HISTORY_LINES 5
#define FINSH_USING_SYMTAB
#define FINSH_USING_DESCRIPTION
#define FINSH_THREAD_PRIORITY 20
#define FINSH_THREAD_STACK_SIZE 4096
#define FINSH_CMD_SIZE 80
#define FINSH_USING_MSH
#define FINSH_USING_MSH_DEFAULT
#define FINSH_ARG_MAX 10

/* Device virtual file system */

#define RT_USING_DFS
#define DFS_USING_WORKDIR
#define DFS_FILESYSTEMS_MAX 2
#define DFS_FILESYSTEM_TYPES_MAX 2
#define DFS_FD_MAX 16
#define RT_USING_DFS_DEVFS

/* Device Drivers */

#define RT_USING_DEVICE_IPC
#define RT_PIPE_BUFSZ 512
#define RT_USING_SERIAL
#define RT_SERIAL_USING_DMA
#define RT_USING_PIN

/* Using WiFi */


/* Using USB */


/* POSIX layer and C standard library */

#define RT_USING_LIBC
#define RT_USING_POSIX

/* Network */

/* Socket abstraction layer */


/* light weight TCP/IP stack */


/* Modbus master and slave stack */


/* AT commands */


/* VBUS(Virtual Software BUS) */


/* Utilities */


/* RT-Thread online packages */

/* IoT - internet of things */


/* Wi-Fi */

/* Marvell WiFi */


/* Wiced WiFi */


/* IoT Cloud */

#define PKG_USING_NIMBLE

/* Bluetooth Role support */

#define PKG_NIMBLE_ROLE_PERIPHERAL
#define PKG_NIMBLE_ROLE_CENTRAL
#define PKG_NIMBLE_ROLE_BROADCASTER
#define PKG_NIMBLE_ROLE_OBSERVER

/* Host Stack Configuration */

#define PKG_NIMBLE_HOST
#define PKG_NIMBLE_HOST_THREAD_STACK_SIZE 1536
#define PKG_NIMBLE_HOST_THREAD_PRIORITY 6

/* Controller Configuration */

#define PKG_NIMBLE_CTLR
#define PKG_NIMBLE_CTLR_THREAD_STACK_SIZE 1024
#define PKG_NIMBLE_CTLR_THREAD_PRIORITY 7
#define PKG_NIMBLE_BSP_NRF52

/* Bluetooth Mesh support */


/* HCI Transport support */

/* Device Driver support */

#define NIMBLE_DEBUG_LEVEL_I
#define NIMBLE_DEBUG_LEVEL 2
#define PKG_NIMBLE_SAMPLE_PER_HR
#define PKG_NIMBLE_MAX_CONNECTIONS 1
#define PKG_NIMBLE_WHITELIST
#define PKG_NIMBLE_MULTI_ADV_INSTANCES 0
#define PKG_USING_NIMBLE_LATEST_VERSION

/* security packages */


/* language packages */


/* multimedia packages */


/* tools packages */


/* system packages */


/* peripheral libraries and drivers */

/* sensors drivers */


/* miscellaneous packages */


/* samples: kernel and components samples */


/* Privated Packages of RealThread */


/* Network Utilities */

#define NIMBT_ROLE_PERIPHERAL
#define NIMBT_ROLE_BROADCASTER

/* Observer */


/* Common configuration */

/* Host stack configuration */

#define NIMBLE_HOST

/* Controller configuretion */

#define NIMBLE_CTLR
#define NIMBLE_BSP_NRF52
#define NIMBLE_SAMPLE_PER_HR

#endif
