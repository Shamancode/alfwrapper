#include <alfwrapper/cleanup.h>  // for _cleanup_free_
#include <alfwrapper/data.h>     // for address6, address4, interface, subnet4
#include <alfwrapper/die.h>      // for die
#include <alfwrapper/parse.h>    // for key_value_type, mapspec
#include <alfwrapper/socket.h>   // for socktype
#include <alfwrapper/string.h>   // for string_equal, string_duplicate
#include <arpa/inet.h>           // for inet_pton
#include <endian.h>              // for be64toh, be32toh
#include <net/if.h>              // for if_nametoindex
#include <stdbool.h>             // for false
#include <stdlib.h>              // for atoi
#include <string.h>              // for strsep
#include <sys/socket.h>          // for AF_INET, AF_INET6, SOCK_DGRAM, SOCK_...

static void parse_address4 (const char* input, struct address4* output) {
	int r = inet_pton (AF_INET, input, output);

	if (r != 1) {
		die ("inet_pton (AF_INET, %s, %p) failed", input, output);
	}

	/* Yes, we want it that way */
	output->raw = be32toh (output->raw);
}

static void parse_address6 (const char* input, struct address6* output) {
	int r = inet_pton (AF_INET6, input, output);

	if (r != 1) {
		die ("inet_pton (AF_INET6, %s, %p) failed", input, output);
	}

	/* Yes, we want it that way */
	output->high = be64toh (output->high);
	output->low  = be64toh (output->low);
}

static void parse_subnet4 (const char* input, struct subnet4* output) {
	char* copy _cleanup_free_ = string_duplicate (input);
	char* data = copy;

	char* prefix = strsep (&data, "/");
	parse_address4 (prefix, &output->address);
	output->prefix = atoi (data);
}

static void parse_subnet6 (const char* input, struct subnet6* output) {
	char* copy _cleanup_free_ = string_duplicate (input);
	char* data = copy;

	char* prefix = strsep (&data, "/");
	parse_address6 (prefix, &output->address);
	output->prefix = atoi (data);
}

static void parse_interface (const char* input, struct interface* output) {
	output->raw = if_nametoindex (input);

	if (output->raw == 0) {
		die ("if_nametoindex(%s) failed", input);
	}
}

static void parse_portnumber (const char* input, struct portnumber* output) {
	output->raw = atoi (input);
}

static void parse_index (const char* input, struct index* output) {
	output->raw = atoi (input);
}

static void parse_typed (const char* input, union key_value_type* output) {
	char* copy _cleanup_free_ = string_duplicate (input);
	char* data = copy;

	char* type  = strsep (&data, ":");

	if (!type || !data) {
		die ("Invalid typed data: %s", input);
	}

	if (false) {}
	else if (string_equal (type, "address4"))   { parse_address4   (data, &output->address4_member);   }
	else if (string_equal (type, "address6"))   { parse_address6   (data, &output->address6_member);   }
	else if (string_equal (type, "index"))      { parse_index      (data, &output->index_member);      }
	else if (string_equal (type, "interface"))  { parse_interface  (data, &output->interface_member);  }
	else if (string_equal (type, "portnumber")) { parse_portnumber (data, &output->portnumber_member); }
	else if (string_equal (type, "subnet4"))    { parse_subnet4    (data, &output->subnet4_member);    }
	else if (string_equal (type, "subnet6"))    { parse_subnet6    (data, &output->subnet6_member);    }
	else { die ("parse_typed (%s, %p): Unrecognized type '%s'", input, output, type); }
}

void parse_mapspec (const char* input, mapspec* output) {
	char* copy _cleanup_free_ = string_duplicate (input);
	char* data = copy;

	char* map = strsep (&data, ",");
	char* key = strsep (&data, ",");
	char* val = strsep (&data, ",");

	if (!map || !key || !val) {
		die ("Invalid map specification: %s", input);
	}

	output->map = string_duplicate (map);
	parse_typed (key, &output->key);
	parse_typed (val, &output->val);
}

void parse_socktype (const char* input, socktype* output) {
	if (string_equal (input, "SOCK_DGRAM")) {
		*output = SOCK_DGRAM;
	} else if (string_equal (input, "SOCK_STREAM")) {
		*output = SOCK_STREAM;
	} else {
		die ("parse_socktype (%s, %p): Unknown socket type", input, output);
	}
}
