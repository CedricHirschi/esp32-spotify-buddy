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

    size_t size(const char *key)
    {
        size_t required_size;
        esp_err_t ret;
        ret = nvs_get_str(nvsHandle, key, NULL, &required_size);
        if (ret == ESP_ERR_NVS_NOT_FOUND)
            return 0;
        return required_size;
    }

    const char *get(const char *key)
    {
        size_t required_size = size(key);
        if (required_size == 0)
            return NULL;

        char *value = (char *)malloc(required_size);
        ESP_ERROR_CHECK(nvs_get_str(nvsHandle, key, value, &required_size));

        return value;
    }

    bool get(const char *key, char *value, size_t value_size)
    {
        size_t required_size = size(key);
        if (required_size == 0)
            return false;
        assert(required_size <= value_size);

        ESP_ERROR_CHECK(nvs_get_str(nvsHandle, key, value, &required_size));

        return true;
    }

    void eraseAll()
    {
        ESP_ERROR_CHECK(nvs_erase_all(nvsHandle));
        ESP_ERROR_CHECK(nvs_commit(nvsHandle));
    }
};

#endif // NVSHANDLER_H