#ifndef __CN100_TIME_H__
#define __CN100_TIME_H__

#define CN100_TX_TIMEOUT 			(HZ*2)				/* tx packet time-out time 1.5 s" */

#ifdef _CN100_MII_
#define CN100_TIMER_WUT  			jiffies+(HZ>>4)		/* timer wakeup time : 2 second */
#endif

#ifdef _RESERVED_MEM_
#define CN100_TIMER_INTERRUPT  	jiffies+(HZ)
#endif

void cn100_timeout 		( struct net_device *dev );
#ifdef _CN100_MII_
int init_eth_timer 		( struct net_device *dev );
void release_eth_timer 	( struct board_info * db );
#endif

#endif /* __CN100_TIME_H__ */
