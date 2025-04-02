idf_component_register(
    SRCS
        "trailcam.c"
        "camera.c"
        "nvs_storage.c"
        "sd_card.c"
        "wifi.c"
        "webserver/webserver.c"
        "webserver/root_handler.c"
        "webserver/config_manager.c"
        "webserver/file_browser.c"
    INCLUDE_DIRS ".")
