phytool
=======

Linux MDIO register access

Usage
-----

Unlike the original one, PHYAD was automatically filled, So read C22 register or C45 MMD.register directly.
Usage:
      phytool -r [ --read ] eth0 [MMD.]REG       Read value from register
      phytool -w [ --write ] eth0 [MMD.]REG val  Write value to register
      phytool -p [ --print ] eth0                Print value
      phytool -d [ --dump ] eth0 desc.yaml       Dump all registers provided in yaml

Clause 22:

REG  := <0-0x1f>

Clause 45 (not supported by all MDIO drivers):

MMD  := <0-0x1f>
REG  := <0-0xffff>

Bug report address: https://github.com/junka/phytool/issues

Examples
--------

~ # phytool read eth4 4
0x0de1

~ # phytool dump eth0 phychips/mv88q2110.yaml
MV88Q2110: Marvell
Group IEEE PMA/PMD Registers, MMD 1
phy_read: 32801, reg 00x0000                                      PMA/PMD Control Register 1    0x0040
                Reset: 0
                Speed Select: 2
                Low Power: 0
                PMA loopback: 0
phy_read: 32801, reg 20x0002                            PMA/PMD Device Identifier Register 1    0x002b
phy_read: 32801, reg 30x0003                            PMA/PMD Device Identifier Register 2    0x0983
                OUI: 0
                Model Number: 0
                Revision Number: 0
phy_read: 32801, reg 40x0004                                  PMA/PMD Speed Ability Register    0x0010
                10M Capable: 0
                100M Capable: 0
                1000M Capable: 1
                10G Capable: 0
phy_read: 32801, reg 50x0005                           PMA/PMD Devices In Package Register 1    0x008a
                Auto-Negotiation Present: 1
                DTS XS Present: 0
                PHY XS Present: 0
                PCS Present: 1
                WIS Present: 0
                PMD/PMA Present: 1
                Clause 22 Registers Present: 0
phy_read: 32801, reg 60x0006                           PMA/PMD Devices In Package Register 2    0x0000
phy_read: 32801, reg e0x000e                           PMA/PMD Package Identifier Register 1    0x002b
phy_read: 32801, reg f0x000f                           PMA/PMD Package Identifier Register 2    0x0983
phy_read: 32801, reg 120x0012                      BASE-T1 PMD/PMA Extended Ability Register    0x0003
                1000BASE-T1 Ability: 1
                100BASE-T1 Ability: 1
phy_read: 32801, reg 8340x0834                              BASE-T1 PMD/PMA Control Register    0x8001
                Master/Slave Config Value: 0
                Type Selection: 0
phy_read: 32801, reg 8360x0836                      100BASE-T1 PMA/PMD Test Control Register    0x0000
                100BASE-T1 Test Mode Control: 0
phy_read: 32801, reg 9000x0900                                      BASE-T1 Control Register    0x0000
                Reset: 0
                Global PMA Transmit Disable: 0
                Low Power: 0
phy_read: 32801, reg 9010x0901                               1000BASE-T1 PMA Status Register    0x0d04
                Receive Fault Ability: 0
                Low Power Ability: 1
                Received Polarity: 1
                Receive Fault: 0
                Receive Link Status: 0
Origin & References
-------------------

phytool is developed and maintained by Tobias Waldekranz.  
Please file bug fixes and pull requests at [GitHub][]

[GitHub]: https://github.com/wkz/phytool
