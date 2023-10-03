#ifndef NVSHANDLER_H
#define NVSHANDLER_H

#include <Arduino.h>
#include <nvs_flash.h>

class NVSHandler
{
private:
    nvs_handle nvsHandle;

public:
    NVSHandler()
    {
        nvs_flash_init_partition("nvs");
        nvs_open_from_partition("nvs", "storage", NVS_READWRITE, &nvsHandle);
    }

    ~NVSHandler()
    {
        nvs_close(nvsHandle);
    }

    void set(const char *key, const char *value)
    {
        nvs_set_str(nvsHandle, key, value);
        nvs_commit(nvsHandle);
    }

    const char *get(const char *key)
    {
        size_t required_size;
        nvs_get_str(nvsHandle, key, NULL, &required_size);

        char *value = (char *)malloc(required_size);
        nvs_get_str(nvsHandle, key, value, &required_size);

        return value;
    }

    void eraseAll()
    {
        nvs_erase_all(nvsHandle);
        nvs_commit(nvsHandle);
    }
};

#endif // NVSHANDLER_H