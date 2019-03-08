#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>

#include <argz.h>
#include <envz.h>

#include "config.h"
#include "log.h"
#include "utils.h"

int config_init(struct config *config, const char *path)
{
	int ret;
	char __attribute__((cleanup(string_cleanup)))*string = NULL;
	FILE __attribute__((cleanup(file_cleanup)))*f = NULL;
	long size;
	size_t sret;

	memset(config, 0, sizeof(*config));

	config->path = strdup(path);
	if (config->path == NULL)
		return -errno;

	f = fopen(path, "rbe");
	if (f == NULL)
		return -errno;

	/* compute the size of the file */
	ret = fseek(f, 0, SEEK_END);
	if (ret == -1)
		return -errno;
	size = ftell(f);
	if (size == -1)
		return -errno;
	ret = fseek(f, 0, SEEK_SET);
	if (ret == -1)
		return -errno;

	/* read all */
	string = calloc(size, 1);
	if (string == NULL)
		return -errno;

	sret = fread(string, 1, size, f);
	if (sret < (size_t)size)
		return feof(f) ? -EIO : ret;

	return -argz_create_sep(string, '\n', &config->argz, &config->len);
}

const char *config_get(const struct config *config, const char *key)
{
	return envz_get(config->argz, config->len, key);
}

int config_get_uint8_t(const struct config *config, const char *key)
{
	const char *str_value;
	char *endptr;
	unsigned long value;

	str_value = config_get(config, key);
	if (str_value == NULL)
		return errno != 0 ? -errno : -ESRCH;

	value = strtoul(str_value, &endptr, 0);
	if (*str_value == '\0' || *endptr != '\0')
		return -EINVAL;

	if (value > UINT8_MAX)
		return -ERANGE;

	return value;
}


void config_cleanup(struct config *config)
{
	if (config->argz != NULL)
		free(config->argz);
	if (config->path != NULL)
		free(config->path);
	memset(config, 0, sizeof(*config));
}
