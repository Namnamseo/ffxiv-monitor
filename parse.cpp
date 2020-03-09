#include <windows.h>
#include <stdio.h>
#define ZLIB_WINAPI
#include <zlib.h>

#include "parse.h"
#include "util.h"
#include "const.h"

const UINT32 MAGIC = 0x5252a041;

enum OPCODES : UINT16 {
	OPCODE_GROUPMSG = 0x0065,
	OPCODE_DESYNTH_RESULT = 0x025D,
};

static BYTE *zbuf;

void prepare_parse()
{
	zbuf = new BYTE[BUFSIZE];
}

static void parse_groupmsg(BYTE *buf, int len)
{
	// TODO
}

static void parse_desynth_result(BYTE *buf, int len)
{
	UINT16 fromId = readHost16(buf);
	UINT16 fromAttr = readHost16(buf+2);
	UINT16 toId[3], toAttr[3], toCnt[3];
	for(int i = 0; i < 3; ++i) {
		toId[i] = readHost16(buf+4+8*i);
		toAttr[i] = readHost16(buf+6+8*i);
		toCnt[i] = readHost16(buf+8+8*i);
	}
	UINT16 unknown = readHost32(buf+0x20);

	printf("Desynthesis: ID %d (%c) -> (unknown %x)\n", fromId, ("Q."[!(fromAttr & 0x0F)]), unknown);
	for(int i = 0; i < 3; ++i) {
		if (!toId[i]) continue;
		printf("....ID %d (%c) * %d\n", toId[i], ("Q."[!(toAttr[i] & 0x0F)]), toCnt[i]);
	}
}

static void parse_msg(int idx, BYTE *buf, int len)
{
	UINT32 mlen = readHost32(buf);

	UINT32 usr_src = readHost32(buf+4);
	UINT32 usr_tgt = readHost32(buf+8);

	UINT16 seg_type = readHost16(buf+12);
	UINT16 opcode = readHost16(buf+18);

	if (seg_type == 7) {
		// client keep-alive
		return;
	}
	if (seg_type == 8) {
		// server keep-alive
		return;
	}
	if (seg_type != 3) {
		printf("Non-IPC packet of type %u\n", seg_type);
		return;
	}

	UINT16 ipc_magic = readHost16(buf+16);

	if (ipc_magic != 0x0014) {
		printf("IPC magic %u != 14\n", ipc_magic);
		return;
	}

	BYTE *body = buf + 32;
	int bodylen = mlen - 32;

	if (opcode == OPCODE_GROUPMSG) {
		parse_groupmsg(body, bodylen);
	} else if (opcode == OPCODE_DESYNTH_RESULT) {
		parse_desynth_result(body, bodylen);
	} else {
		printf("conn %d: src %08x tgt %08x op %04x (unknown) len %3d\n",
			idx, usr_src, usr_tgt, opcode, bodylen);
	}
}

static UINT32 decompress(int idx, BYTE *buf, int len)
{
	int ret;
	unsigned have;
	z_stream strm;

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit(&strm);
	if (ret != Z_OK) {
		printf("conn %d: inflateInit() error: %d\n",
			idx, ret);
		return 0;
	}

	strm.next_in = buf;
	strm.avail_in = len;

	strm.next_out = zbuf;
	strm.avail_out = BUFSIZE;

	ret = inflate(&strm, Z_FINISH);
	if (ret != Z_STREAM_END) {
		printf("conn %d: inflate() error: %d\n",
			idx, ret);
		return 0;
	}

	UINT32 res = BUFSIZE - strm.avail_out;
	return res;
}

static void parse_payload(int idx, BYTE *buf, int len)
{
	if (len < 40) {
		if (len > 0) {
			printf("conn %d: packet %d too small, dropping\n",
				idx, len);
		}
		return;
	}

	while (len >= 40) {
		int magic = readNet32(buf);
		if (magic != MAGIC) {
			// Check 16-zero case.
			for (int j = 0; j < 16; ++j) {
				if (buf[j]) {
					printf("conn %d: magic %08x != %08x (expected)\n",
						idx, magic, MAGIC);
					return;
				}
			}
		}

		UINT64 epoch = readHost64(buf+16);

		UINT16 paylen = readHost16(buf+24);

		if (paylen > len) {
			printf("conn %d: pl length: %u > %d\n",
				idx, paylen, len);
			return;
		}

		UINT16 msgcnt = readHost16(buf+30);
		UINT16 encoding = readHost16(buf+32);

		BYTE *dest;
		UINT32 dlen;

		if (encoding == 0x0000 || encoding == 0x0001)  {
			// not compressed.
			dest = buf + 40;
			dlen = len - 40;
		} else if (encoding == 0x0100 || encoding == 0x0101) {
			dlen = decompress(idx, buf+40, len-40);
			dest = zbuf;
		} else {
			printf("conn %d: unknown encoding %04x\n", idx, encoding);
			return;
		}

		UINT32 off = 0;
		for (int i = 0; i < msgcnt; ++i)
		{
			if (off >= dlen) {
				printf("conn %d: invalid offset %u\n", idx, off);
				return;
			}

			parse_msg(idx, dest+off, dlen-off);

			UINT32 msgLen = readHost32(dest+off);
			off += msgLen;
		}

		buf += paylen;
		len -= paylen;
	}
}

static void parse_tcp_packet(int idx, BYTE *buf, int len)
{
	int hlen = (buf[12] >> 4) * 4;
	int psh = !!(buf[13] & 0x08);
	int paylen = len - hlen;
	if (paylen < 0) {
		printf("conn %d: payload length %d is invalid\n",
			idx, paylen);
		return;
	}
	parse_payload(idx, buf+hlen, len-hlen);
}

void parse_ip_packet(int idx, BYTE *buf, int len)
{
	if ((buf[0]>>4) != 4) {
		printf("conn %d: invalid IPv4 packet received\n", idx);
		return;
	}

	int ihl = (buf[0]&0x0F) * 4;
	int psz = readNet16(buf+2);

	if (psz != len) {
		printf("conn %d: length %d != %d calculated size\n",
			idx, len, psz);
		return;
	}

	parse_tcp_packet(idx, buf + ihl, psz-ihl);
}