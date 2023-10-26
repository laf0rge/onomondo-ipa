#pragma once

#include <stdint.h>
#include "length.h"

struct ipa_context;

enum ipa_eim_pkg_type {
	IPA_EIM_PKG_AC,
	IPA_EIM_PKG_ERR,
};

struct ipa_eim_pkg_ac {
	/* GSMA SGP.22 section 4.1 is not very clear about the maximum size of
	 * an activation code. Usually this code will not exceed 255 characters,
	 * but with optional "Delete Notification for Device Change" element
	 * it may exceed 255 characters. */
	char code[IPA_LEN_AC];
};

struct ipa_eim_pkg {
	enum ipa_eim_pkg_type type;
	union {
		struct ipa_eim_pkg_ac ac;
		int error;
	} u;
};

struct ipa_eim_pkg *ipa_esipa_get_eim_pkg(struct ipa_context *ctx, uint8_t *eid);
void ipa_dump_eim_pkg(struct ipa_eim_pkg *eim_pkg, uint8_t indent,
			  enum log_subsys log_subsys, enum log_level log_level);
void ipa_free_eim_pkg(struct ipa_eim_pkg *eim_pkg);
