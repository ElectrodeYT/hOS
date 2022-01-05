#include <kernel-drivers/CharDevices.h>
#include <debug/klog.h>
#include <CPP/string.h>

namespace Kernel {

int CharDeviceManager::RegisterCharDevice(CharDevice* device) {
    if(!device) { return -1; }
    device->char_device_id = next_id++;
    devices.push_back(device);

    if(!klog_device) { klog_device = device; }
    return device->char_device_id;
}

void CharDeviceManager::DestroyCharDevice(int id) {
    if(klog_device && klog_device->char_device_id == id) { klog_device = NULL; }
    for(size_t i = 0; i < devices.size(); i++) {
        if(devices.at(i)->char_device_id == id) {
            delete devices.at(i);
            devices.remove(i);
            return;
        }
    }
}

CharDevice* CharDeviceManager::get(int id) {
    if(klog_device && klog_device->char_device_id == id) { return klog_device; }
    for(size_t i = 0; i < devices.size(); i++) {
        if(devices.at(i)->char_device_id == id) {
            return devices.at(i);
        }
    }
    return NULL;
}

void CharDeviceManager::KLog_puts(const char* s) {
    if(klog_device) {
        size_t len = strlen(s);
        klog_device->write(s, len);
    }
}

int CharDevice::read(char* buf, size_t len) {
    return read_drv(buf, len);
}

int CharDevice::write(const char* buf, size_t len) {
    return write_drv(buf, len);
}

void KLog_puts_wrapper(void* arg, const char* s) {
    CharDeviceManager* manager = (CharDeviceManager*)arg;
    manager->KLog_puts(s);
}

CharDeviceManager::CharDeviceManager() {
    // Register us in KLog
    KLog::the().registerCallback(KLog_puts_wrapper, this);
}

}