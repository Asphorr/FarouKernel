#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

class DevFsDevice {
public:
    typedef std::shared_ptr<DevFsDevice> Ptr;

    explicit DevFsDevice(const std::string& name, uint8_t major, uint8_t minor, bool isDirectory = false);
    virtual ~DevFsDevice();

    static Ptr create(const std::string& name, uint8_t major, uint8_t minor, bool isDirectory = false);

    inline const std::string& getName() const { return m_name; };
    inline uint8_t getMajor() const { return m_major; };
    inline uint8_t getMinor() const { return m_minor; };
    inline bool isDirectory() const { return m_isDirectory; };

protected:
    std::string m_name;
    uint8_t m_major;
    uint8_t m_minor;
    bool m_isDirectory;
};

class DevFsFile : public DevFsDevice {
public:
    typedef std::shared_ptr<DevFsFile> Ptr;

    explicit DevFsFile(const std::string& name, uint8_t major, uint8_t minor);
    virtual ~DevFsFile();

    static Ptr create(const std::string& name, uint8_t major, uint8_t minor);

    virtual int open(int flags);
    virtual int close();
    virtual ssize_t read(char* buf, size_t count, off_t offset);
    virtual ssize_t write(const char* buf, size_t count, off_t offset);
    virtual int ioctl(unsigned int request, unsigned long argument);
    virtual int mmap(struct vm_area_struct* vma);

protected:
    int m_flags;
};

class DevFsDirectory : public DevFsDevice {
public:
    typedef std::shared_ptr<DevFsDirectory> Ptr;

    explicit DevFsDirectory(const std::string& name, uint8_t major, uint8_t minor);
    virtual ~DevFsDirectory();

    static Ptr create(const std::string& name, uint8_t major, uint8_t minor);

    virtual int open(int flags);
    virtual int close();
    virtual ssize_t read(char* buf, size_t count, off_t offset);
    virtual ssize_t write(const char* buf, size_t count, off_t offset);
    virtual int ioctl(unsigned int request, unsigned long argument);
    virtual int mmap(struct vm_area_struct* vma);

    virtual int addChild(Ptr child);
    virtual int removeChild(Ptr child);

protected:
    std::vector<Ptr> m_children;
};

class DevFsEventHandler {
public:
    typedef std::shared_ptr<DevFsEventHandler> Ptr;

    explicit DevFsEventHandler(std::function<void(uint32_t)> callback);
    virtual ~DevFsEventHandler();

    static Ptr create(std::function<void(uint32_t)> callback);

    virtual void handleEvent(uint32_t event);

protected:
    std::function<void(uint32_t)> m_callback;
};

class DevFsManager {
public:
    typedef std::shared_ptr<DevFsManager> Ptr;

    static Ptr create();

    virtual int init();
    virtual int deInit();

    virtual int mount(const std::string& path);
    virtual int umount(const std::string& path);

    virtual int addDevice(DevFsDevice::Ptr device);
    virtual int removeDevice(DevFsDevice::Ptr device);

    virtual int addEventHandler(DevFsEventHandler::Ptr handler);
    virtual int removeEventHandler(DevFsEventHandler::Ptr handler);

protected:
    std::vector<DevFsDevice::Ptr> m_devices;
    std::vector<DevFsEventHandler::Ptr> m_handlers;
};
