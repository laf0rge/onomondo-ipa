/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 *
 */

#include <stdio.h>
#include <stdbool.h>
#include <onomondo/ipa/utils.h>
#include <asn_application.h>
#include "src/ipa/libipa/utils.h"
#include "src/ipa/libipa/es10x.h"
#include "src/ipa/libipa/bpp_segments.h"

void ipa_bpp_segments_encode_test(char *test_vector_path)
{
	uint8_t bpp[20480];
	FILE *bpp_file = NULL;
	size_t bpp_len;
	asn_dec_rval_t rc;
	struct BoundProfilePackage *bpp_dec = NULL;
	struct ipa_bpp_segments	*segments = NULL;
	
	/* Load test BPP test vector from file */
	assert(test_vector_path);
	bpp_file = fopen(test_vector_path, "r");
	assert(bpp_file);
	bpp_len = fread(&bpp, sizeof(char), sizeof(bpp), bpp_file);
	fclose(bpp_file);
	printf("bpp size: %ld\n", bpp_len);

	/* Decode BPP test vector */
	rc = ber_decode(0, &asn_DEF_BoundProfilePackage,
			(void **)&bpp_dec, bpp, bpp_len);
	assert (rc.code == RC_OK);
	
	/* Generate BPP segments */
	segments = ipa_bpp_segments_encode(bpp_dec);
	assert(segments);
	ipa_bpp_segments_free(segments);
	
	ASN_STRUCT_FREE(asn_DEF_BoundProfilePackage, bpp_dec);
}

int main(int argc, char **argv)
{
	ipa_bpp_segments_encode_test(argv[1]);
	return 0;
}
