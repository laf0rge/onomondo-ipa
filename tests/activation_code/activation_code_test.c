/*
 * Author: Philipp Maier <pmaier@sysmocom.de> / sysmocom - s.f.m.c. GmbH
 *
 */

#include <stdio.h>
#include "src/ipa/libipa/activation_code.h"

void parse_ac(char *ac)
{
	struct ipa_activation_code *ac_decoded;
	printf("Encoded activation code: \"%s\"\n", ac);
	ac_decoded = ipa_activation_code_parse(ac);
	ipa_activation_code_dump(ac_decoded, 0, SMAIN, LINFO);
	ipa_activation_code_free(ac_decoded);
	printf("\n");
}

int main(int argc, char **argv)
{
	parse_ac("1$SMDP.EXAMPLE.COM$04386-AGYFT-A74Y8-3F815");
	parse_ac("1$SMDP.EXAMPLE.COM$04386-AGYFT-A74Y8-3F815$$1");
	parse_ac("1$SMDP.EXAMPLE.COM$04386-AGYFT-A74Y8-3F815$1.3.6.1.4.1.31746$1");
	parse_ac("1$SMDP.EXAMPLE.COM$04386-AGYFT-A74Y8-3F815$1.3.6.1.4.1.31746");
	parse_ac("1$SMDP.EXAMPLE.COM$$1.3.6.1.4.1.31746");
	parse_ac("1$SMDP.EXAMPLE.NET$KL14XA-8C7RLY$1.3.6.1.4.1.3174");
	parse_ac("1$SMDP.EXAMPLE.COM$$$$$$$$$$$$$$$$$$$$$$$$$$");

	/* When the activation code is provided via a QR code it has to be prefixed
	 * with "LPA:", see also GSMA SGP.22, section 4.1 */
	parse_ac("LPA:1$SMDP.EXAMPLE.COM$04386-AGYFT-A74Y8-3F815");

	/* Invalid */
	parse_ac("xyz14390lojij");
	parse_ac("");
	parse_ac("1$");
	parse_ac("LPA:1$");
	parse_ac("LPA:1$$$$$$$$$$$$$$$$$$$");
	parse_ac("\x10$\xAA$\xBB$\xCC");
	parse_ac("1$SMDP.EXAMPLE.COM");
	parse_ac(NULL);

	return 0;
}
