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

#define _GNU_SOURCE

#include <glob.h>
#include <errno.h>
#include <libgen.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <linux/ethtool.h>
#include <linux/mdio.h>
#include <linux/sockios.h>

#include "phytool.h"

extern char *__progname;

struct applet {
	const char *name;
	int (*usage)(int code);
	int (*parse_loc)(char *text, struct loc *loc, int strict);
	int (*print)(const struct loc *loc, int indent);
};


static int __phy_op(const struct loc *loc, uint16_t *val, int cmd)
{
	static int sd = -1;

	struct ifreq ifr;
	struct mii_ioctl_data* mii = (struct mii_ioctl_data *)(&ifr.ifr_data);
	int err;

	if (sd < 0)
		sd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sd < 0)
		return sd;

	strncpy(ifr.ifr_name, loc->ifnam, sizeof(ifr.ifr_name));

	mii->phy_id  = loc->phy_id;
	mii->reg_num = loc->reg;
	mii->val_in  = *val;
	mii->val_out = 0;

	err = ioctl(sd, cmd, &ifr);
	if (err)
		return -errno;

	*val = mii->val_out;
	return 0;
}

int phy_read(const struct loc *loc)
{
	uint16_t val = 0;
	int err = __phy_op(loc, &val, SIOCGMIIREG);

	if (err) {
		fprintf(stderr, "error: phy_read (%d)\n", err);
		return err;
	}

	return val;
}

int phy_write(const struct loc *loc, uint16_t val)
{
	int err = __phy_op(loc, &val, SIOCSMIIREG);

	if (err)
		fprintf(stderr, "error: phy_write (%d)\n", err);

	return err;
}

uint32_t phy_id(const struct loc *loc)
{
	struct loc loc_id = *loc;
	uint16_t id[2];

	loc_id.reg = MII_PHYSID1;
	id[0] = phy_read(&loc_id);
	
	loc_id.reg = MII_PHYSID2;
	id[1] = phy_read(&loc_id);

	return (id[0] << 16) | id[1];
}

static int get_phyad(const char *name)
{
	char buffer[sizeof(struct ethtool_link_settings) + sizeof(__u32) * 3 * 128];
	struct ethtool_link_settings *link_settings;
	struct ifreq ifr;
	int err;

	int sd = -1;

	link_settings = (struct ethtool_link_settings *)buffer;
	memset(buffer, 0, sizeof(buffer));

	sd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sd < 0)
		return sd;

	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));
	link_settings->cmd = ETHTOOL_GLINKSETTINGS;
	ifr.ifr_data = (caddr_t)link_settings;

	err = ioctl(sd, SIOCETHTOOL, &ifr);
	if (err < 0) {
		close(sd);
		return err;
	}

	/* we expect a strictly negative value from kernel.
	 */
	if (link_settings->link_mode_masks_nwords >= 0
	    || link_settings->cmd != ETHTOOL_GLINKSETTINGS)
		return err;

	/* now send the real request */
	link_settings->cmd = ETHTOOL_GLINKSETTINGS;
	link_settings->link_mode_masks_nwords = -link_settings->link_mode_masks_nwords;

	err = ioctl(sd, SIOCETHTOOL, &ifr);
	if (err < 0)
		return err;

	if (link_settings->link_mode_masks_nwords <= 0
	    || link_settings->cmd != ETHTOOL_GLINKSETTINGS)
		return err;

	return link_settings->phy_address;
}


static int loc_segments(char *text, char **a, char **b)
{
	*a = strtok(text, "/");
	if (!*a)
		return 0;

	*b = strtok(NULL, "/");
	if (!*b)
		return 1;

	return 2;
}

static int phytool_parse_loc_segs(char *dev, char *reg,
				  struct loc *loc)
{
	strncpy(loc->ifnam, dev, IFNAMSIZ - 1);

	int pad, mmd;
	char *end;

	pad = get_phyad(dev);
	if (pad < 0) {
		return 1;
	}

	if (!reg) {
		loc->phy_id = pad;
		loc->reg = REG_SUMMARY;
	} else {
		mmd = strtoul(reg, &end, 0);
		if (!end[0]) {
			loc->phy_id = pad;
			loc->reg = mmd;
		} else if (end[0] == '.') {
			loc->phy_id = mdio_phy_id_c45(pad, mmd);
			loc->reg = strtoul(end + 1, NULL, 0);
		} else {
			return 1;
		}
	}

	return 0;
}

static int phytool_parse_loc(char *text, struct loc *loc, int strict)
{
	char *dev = NULL, *reg = NULL;
	int segs;

	segs = loc_segments(text, &dev, &reg);
	if (segs < (strict ? 2 : 1))
		return -EINVAL;

	return phytool_parse_loc_segs(dev, reg, loc);
}

static int phytool_read(struct applet *a, int argc, char **argv)
{
	struct loc loc;
	int val;

	if (!argc)
		return 1;

	if (a->parse_loc(argv[0], &loc, 1)) {
		fprintf(stderr, "error: bad location format\n");
		return 1;
	}

	val = phy_read(&loc);
	if (val < 0)
		return 1;

	printf("0x%.4x\n", val);
	return 0;
}

static int phytool_write(struct applet *a, int argc, char **argv)
{
	struct loc loc;
	unsigned long val;
	int err;

	if (argc < 2)
		return 1;

	if (a->parse_loc(argv[0], &loc, 1)) {
		fprintf(stderr, "error: bad location format\n");
		return 1;
	}

	val = strtoul(argv[1], NULL, 0);

	err = phy_write (&loc, val);
	if (err)
		return 1;

	return 0;
}

static int phytool_print(struct applet *a, int argc, char **argv)
{
	struct loc loc;
	int err;

	if (!argc)
		return 1;

	if (a->parse_loc(argv[0], &loc, 0)) {
		fprintf(stderr, "error: bad location format\n");
		return 1;
	}

	err = a->print(&loc, 0);
	if (err)
		return 1;
	
	return 0;
}

static int phytool_usage(int code)
{
	printf("Usage: %s read  IFACE/[MMD.]REG\n"
	       "       %s write IFACE/[MMD.]REG <0-0xffff>\n"
	       "       %s print IFACE/[[MMD.]REG]\n"
	       "\n"
	       "Clause 22:\n"
	       "\n"
	       "REG  := <0-0x1f>\n"
	       "\n"
	       "Clause 45 (not supported by all MDIO drivers):\n"
	       "\n"
	       "MMD  := <0-0x1f>\n"
	       "REG  := <0-0xffff>\n"
	       "\n"
	       "Examples:\n"
	       "       %s read  eth0/4.0x1000\n"
	       "       %s write eth0/0 0x1140\n"
	       "       %s print eth0/0x1c\n"
	       "\n"
	       "The `read` and `write` commands are simple register level\n"
	       "accessors. The `print` command will pretty-print a register. When\n"
	       "using the `print` command, the register is optional. If left out, the\n"
	       "most common registers will be shown.\n"
	       "\n"
	       "Bug report address: https://github.com/junka/phytool/issues\n"
	       "\n",
	       __progname, __progname, __progname, __progname, __progname, __progname);
	return code;
}

static struct applet applets[] = {
	{
		.name = "phytool",
		.usage = phytool_usage,
		.parse_loc = phytool_parse_loc,
		.print = print_phytool
	},

	{ .name = NULL }
};

int main(int argc, char **argv)
{
	struct applet *a;

	for (a = applets; a->name; a++) {
		if (!strcmp(__progname, a->name))
			break;
	}

	if (!a->name)
		a = applets;

	if (argc < 2)
		return a->usage(1);

	if (!strcmp(argv[1], "read"))
		return phytool_read(a, argc - 2, &argv[2]);
	else if (!strcmp(argv[1], "write"))
		return phytool_write(a, argc - 2, &argv[2]);
	else if (!strcmp(argv[1], "print"))
		return phytool_print(a, argc - 2, &argv[2]);
	else
		return phytool_print(a, argc - 1, &argv[1]);

	fprintf(stderr, "error: unknown command \"%s\"\n", argv[1]);
	return a->usage(1);
}
