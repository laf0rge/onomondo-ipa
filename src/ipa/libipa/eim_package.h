#pragma once

#include <stdint.h>
#include "length.h"

struct ipa_context;

enum ipa_eim_package_type {
	IPA_EIM_PACKAE_AC
};

struct ipa_eim_package_ac {
	/* GSMA SGP.22 section 4.1 is not very clear about the maximum size of
	 * an activation code. Usually this code will not exceed 255 characters,
	 * but with optional "Delete Notification for Device Change" element
	 * it may exceed 255 characters. */
	char code[IPA_LEN_AC];
};

struct ipa_eim_package {
	enum ipa_eim_package_type type;
	union {
		struct ipa_eim_package_ac ac;
	} u;
};

struct ipa_eim_package *ipa_eim_package_fetch(struct ipa_context *ctx,
					      uint8_t *eid);
void ipa_eim_package_dump(struct ipa_eim_package *eim_package, uint8_t indent,
			  enum log_subsys log_subsys, enum log_level log_level);
void ipa_eim_package_free(struct ipa_eim_package *eim_package);
