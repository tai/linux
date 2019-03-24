#ifndef __CN100_INT_H__
#define __CN100_INT_H__

irqreturn_t cn100_interrupt(int irq, void *dev_id);
int cn100_start_xmit(struct sk_buff *skb, struct net_device *dev);
int cn100_poll(struct napi_struct *napi, int budget);

#endif /* __CN100_INT_H__ */
