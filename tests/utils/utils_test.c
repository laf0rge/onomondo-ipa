/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 *
 */

#include <stdio.h>
#include <stdbool.h>
#include <onomondo/ipa/utils.h>
#include "src/ipa/libipa/utils.h"

void ipa_tag_in_taglist_test(void)
{
	uint8_t _tag_list[] = { 0x80, 0xBF, 0x20, 0xBF, 0x22, 0x83, 0x84, 0xA5, 0xA6, 0x88, 0xA9, 0xBF, 0x2B };
	struct ipa_buf *tag_list;
	bool rc;

	tag_list = ipa_buf_alloc_data(sizeof(_tag_list), _tag_list);

	rc = ipa_tag_in_taglist(0x80, tag_list);
	assert(rc == true);
	rc = ipa_tag_in_taglist(0xBF20, tag_list);
	assert(rc == true);
	rc = ipa_tag_in_taglist(0xBF2B, tag_list);
	assert(rc == true);
	rc = ipa_tag_in_taglist(0xA5, tag_list);
	assert(rc == true);
	rc = ipa_tag_in_taglist(0x22, tag_list);
	assert(rc == false);
	rc = ipa_tag_in_taglist(0xBF, tag_list);
	assert(rc == false);
	rc = ipa_tag_in_taglist(0x2B, tag_list);
	assert(rc == false);
	rc = ipa_tag_in_taglist(0xA3, tag_list);
	assert(rc == false);
	rc = ipa_tag_in_taglist(0x81, tag_list);
	assert(rc == false);
	rc = ipa_tag_in_taglist(0xFF, tag_list);
	assert(rc == false);
	rc = ipa_tag_in_taglist(0x00, tag_list);
	assert(rc == false);
	rc = ipa_tag_in_taglist(0xBF23, tag_list);
	assert(rc == false);
	rc = ipa_tag_in_taglist(0xBF00, tag_list);
	assert(rc == false);
	rc = ipa_tag_in_taglist(0xBFFF, tag_list);
	assert(rc == false);

	IPA_FREE(tag_list);
}

int main(int argc, char **argv)
{
	ipa_tag_in_taglist_test();
	return 0;
}
