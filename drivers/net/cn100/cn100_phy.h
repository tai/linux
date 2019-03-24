#ifndef __CN100_PHY_H__
#define __CN100_PHY_H__

int cn100_phy_read	( struct net_device *dev, int phy_id, int reg );
void cn100_phy_write	( struct net_device *dev, int phy_id, int reg, int value_data );

#endif /* __CN100_PHY_H__ */
