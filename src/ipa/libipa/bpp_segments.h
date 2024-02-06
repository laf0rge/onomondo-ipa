#pragma once

#include <onomondo/ipa/utils.h>
#include <BoundProfilePackage.h>

struct ipa_bpp_segments {
	struct ipa_buf **segment;
	size_t count;
};

struct ipa_bpp_segments *ipa_bpp_segments_encode(const struct BoundProfilePackage *bpp);
void ipa_bpp_segments_free(struct ipa_bpp_segments *segments);
