#ifndef _DELIVERY_OPTIMIZATION_CONFIG_INTERNAL_H
#define _DELIVERY_OPTIMIZATION_CONFIG_INTERNAL_H

// Since the public api is declared as extern c, these can't be defined using the same namespace convention (namespace mangling occurs due to declaration of extern c)

int internal_set_iot_connection_string(const char* value);

char* internal_get_components_version();

void internal_free_version_buf(char** ppBuffer);

#endif // _DELIVERY_OPTIMIZATION_INTERNAL_CONFIG_H
