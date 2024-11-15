/* This file is part of phytool
 *
 * phytool is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * phytool is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with phytool.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __PHYTOOL_H
#define __PHYTOOL_H

#include <stdint.h>
#include <net/if.h>
#include <linux/ethtool.h>
#include <linux/mdio.h>

#define INDENT 3

#define REG_SUMMARY 0xffff

struct loc {
	char ifnam[IFNAMSIZ];
	uint16_t phy_id;
	uint16_t reg;
};


struct num_list {
    int *numbers;
    int count;
};

struct reg_field {
    char *field;
    struct num_list offset;
};

struct reg {
    char *name;
    uint32_t addr;
    uint16_t reset;
    int num_field;
    struct reg_field fields[16];
};

struct regcls {
    char *name;
    int dev;
    int num_reg;
    struct reg* regs;
};

struct phy_desc {
    char *name;
    char *manufactory;
    int num_cls;
    struct regcls *regcls;
};


static inline int loc_is_c45(const struct loc *loc)
{
	return loc->phy_id & MDIO_PHY_ID_C45;
}

static inline int loc_c45_dev(const struct loc *loc)
{
	return loc->phy_id & MDIO_PHY_ID_DEVAD;
}

static inline int loc_c45_port(const struct loc *loc)
{
	return (loc->phy_id & MDIO_PHY_ID_PRTAD) >> 5;
}

int      phy_read (const struct loc *loc);
int      phy_write(const struct loc *loc, uint16_t val);
uint32_t phy_id   (const struct loc *loc);

void print_attr_name(const char *name, int indent);
void print_bool(const char *name, int on);

int print_phytool(struct loc *loc, const char *descfile);

uint32_t num_list_value(struct num_list *l, uint32_t val);
struct phy_desc* read_phy_yaml(const char *filename);
void free_phydesc(struct phy_desc*);

#endif	/* __PHYTOOL_H */
