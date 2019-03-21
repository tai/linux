#ifndef __CN100_DBG_H__
#define __CN100_DBG_H__

/* Board/System/Debug information/definition ---------------- */
/**************************************************************/

/* define macro __CN100_DEBUG__ for debug driver				  */
#define __CN100_DEBUG__

#define CARDNAME 				"CN100"

#define CN100_DEBUG 			0

/* dump all debug message */
#if CN100_DEBUG > 2
#define PRINTK3(args...)  	printk(CARDNAME ": " args)
#else
#define PRINTK3(args...)  	do { } while(0)
#endif

#if CN100_DEBUG > 1
#define PRINTK2(args...)  	printk(CARDNAME ": " args)
#else
#define PRINTK2(args...)  	do { } while(0)
#endif

#if CN100_DEBUG > 0
#define PRINTK1(args...)  	printk(CARDNAME ": " args)
#define PRINTK(args...)   	printk(CARDNAME ": " args)
#else
#define PRINTK1(args...)  	do { } while(0)
#define PRINTK(args...)   	printk(KERN_DEBUG args)
#endif
/**************************************************************/

#endif /* __CN100_DBG_H__ */
