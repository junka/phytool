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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <getopt.h>

#include <linux/sockios.h>

#include "phytool.h"

static int __phy_op(const char *name, uint16_t phyid, uint16_t reg, uint16_t *val, int cmd)
{
	static int sd = -1;

	struct ifreq ifr;
	struct mii_ioctl_data* mii = (struct mii_ioctl_data *)(&ifr.ifr_data);
	int err;

	if (sd < 0)
		sd = socket(AF_INET, SOCK_DGRAM, 0);

	if (sd < 0)
		return sd;

	strncpy(ifr.ifr_name, name, sizeof(ifr.ifr_name));

	mii->phy_id  = phyid;
	mii->reg_num = reg;
	mii->val_in  = *val;
	mii->val_out = 0;

	err = ioctl(sd, cmd, &ifr);
	if (err)
		return -errno;

	*val = mii->val_out;
	return 0;
}

int phy_read(const char *name, uint16_t phyid, uint16_t reg)
{
	uint16_t val = 0;
	int err = __phy_op(name, phyid, reg, &val, SIOCGMIIREG);

	if (err) {
		fprintf(stderr, "error: phy_read (%d)\n", err);
		return err;
	}

	return val;
}

int phy_write(const char *name, uint16_t phyid, uint16_t reg, uint16_t val)
{
	int err = __phy_op(name, phyid, reg, &val, SIOCSMIIREG);

	if (err)
		fprintf(stderr, "error: phy_write (%d)\n", err);

	return err;
}

uint32_t phy_identifier(const char *ifname, int phyid)
{
	uint16_t id[2];

	id[0] = phy_read(ifname, phyid, MII_PHYSID1);
	id[1] = phy_read(ifname, phyid, MII_PHYSID2);

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
		printf("unable to ioctl for %s\n", name);
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

static int phytool_phyid(const char *ifname, char *regstr, uint16_t *regv)
{
	int pad, mmd;
	char *end;
	int phyid;

	pad = get_phyad(ifname);
	if (pad < 0) {
		return -1;
	}

	if (!regstr) {
		phyid = pad;
		*regv = REG_SUMMARY;
	} else {
		mmd = strtoul(regstr, &end, 0);
		if (!end[0]) {
			phyid = pad;
			*regv = mmd;
		} else if (end[0] == '.') {
			phyid = mdio_phy_id_c45(pad, mmd);
			*regv = strtoul(end + 1, NULL, 0);
		} else {
			return -1;
		}
	}

	return phyid;
}

static int phytool_usage(int code)
{
	printf("Usage:\n"
		   "        -r [ --read ] eth0 [MMD.]REG       Read value from register\n"
	       "        -w [ --write ] eth0 [MMD.]REG val  Write value to register\n"
	       "        -p [ --print ] eth0                Print value\n"
	       "        -d [ --dump ] eth0 desc.yaml       Dump all registers provided in yaml\n"
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
	       "Bug report address: https://github.com/junka/phytool/issues\n"
	       "\n");
	return code;
}

int main(int argc, char **argv)
{
	static struct option long_options[] = {
		{"read", required_argument, NULL, 'r'},
		{"write", required_argument, NULL, 'w'},
		{"print", required_argument, NULL, 'p'},
		{"dump", required_argument, NULL, 'd'},
		{"help", no_argument, NULL, 'h'},
		{0, 0, 0, 0}
	};

	char *ifname, *regstr;
	int phyid;
	uint16_t reg;
	int opt, err;

	int val;
	while ((opt = getopt_long(argc, argv, "r:w:p:d:h", long_options, NULL))!= -1) {
		switch (opt) {
			case 'r':
				ifname = optarg;
				regstr = argv[optind];
				phyid = phytool_phyid(ifname, regstr, &reg);
				if (phyid == -1) {
					printf("no valid phyid\n");
					return -1;
				}
				val = phy_read(ifname, phyid, reg);
				printf("0x%x\n", val);
				break;
			case 'w':
				ifname = optarg;
				regstr = argv[optind];
				val = strtoul(argv[optind+1], NULL, 0);
				phyid = phytool_phyid(ifname, regstr, &reg);
				if (phyid == -1) {
					printf("no valid phyid\n");
					return -1;
				}
				err = phy_write(ifname, phyid, reg, val);
				if (err) {
					printf("Fail to write\n");
					return 1;
				}
				printf("Success\n");
				break;
			case 'p':
				ifname = optarg;
				regstr = argv[optind];
				phyid = phytool_phyid(ifname, regstr, &reg);
				if (phyid == -1) {
					printf("no valid phyid\n");
					return -1;
				}
				err = print_phytool(ifname, phyid, NULL);
				if (err) {
					printf("Fail to print\n");
					return 1;
				}
				break;
			case 'd':
				ifname = optarg;
				regstr = "0";
				phyid = phytool_phyid(ifname, regstr, &reg);
				err = print_phytool(ifname, phyid, argv[optind]);
				if (err) {
					printf("Fail to dump\n");
					return 1;
				}
				break;
			case 'h':
				phytool_usage(0);
				break;
			case '?':
				phytool_usage(0);
				break;
			default:
				fprintf(stderr, "error: unknown command \"%s\"\n", argv[1]);
				phytool_usage(1);
				break;
		}
	}

	if (argc < 2)
		return phytool_usage(1);

	return 0;
}
