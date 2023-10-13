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
        ESP_ERROR_CHECK(nvs_flash_init_partition("nvs"));
        ESP_ERROR_CHECK(nvs_open_from_partition("nvs", "storage", NVS_READWRITE, &nvsHandle));
    }

    ~NVSHandler()
    {
        nvs_close(nvsHandle);
    }

    void set(const char *key, const char *value)
    {
        ESP_ERROR_CHECK(nvs_set_str(nvsHandle, key, value));
        ESP_ERROR_CHECK(nvs_commit(nvsHandle));
    }

    const char *get(const char *key)
    {
        size_t required_size;
        ESP_ERROR_CHECK(nvs_get_str(nvsHandle, key, NULL, &required_size));

        char *value = (char *)malloc(required_size);
        ESP_ERROR_CHECK(nvs_get_str(nvsHandle, key, value, &required_size));

        return value;
    }

    void get(const char *key, char *value, size_t value_size)
    {
        size_t required_size;
        ESP_ERROR_CHECK(nvs_get_str(nvsHandle, key, NULL, &required_size));
        assert(required_size <= value_size);

        ESP_ERROR_CHECK(nvs_get_str(nvsHandle, key, value, &required_size));
    }

    void eraseAll()
    {
        ESP_ERROR_CHECK(nvs_erase_all(nvsHandle));
        ESP_ERROR_CHECK(nvs_commit(nvsHandle));
    }
};

#endif // NVSHANDLER_H