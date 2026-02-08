#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/Foundation.h>
#import <mach-o/loader.h>
#import <mach-o/fat.h>
#import <mach-o/dyld.h>
#import <mach-o/dyld_images.h>
#import <mach/mach.h>
#import <mach/vm_page_size.h>
#import <mach/task_info.h>
#import <mach/mach_traps.h>
#import <stdio.h>
#import <stdlib.h>
#import <libgen.h>
#import <limits.h>
#include <string.h>

// libproc function declarations
extern "C" {
    int proc_listallpids(void* buffer, int buffersize);
    int proc_name(int pid, void* buffer, uint32_t buffersize);
}

// Embedded Vector3.h content
struct Vector3
{
    union
    {
        struct
        {
            float x;
            float y;
            float z;
        };
        float data[3];
    };

    inline Vector3();
    inline Vector3(float data[]);
    inline Vector3(float value);
    inline Vector3(float x, float y);
    inline Vector3(float x, float y, float z);

    static inline Vector3 zero();
    static inline Vector3 One();
    static inline Vector3 Right();
    static inline Vector3 Left();
    static inline Vector3 Up();
    static inline Vector3 Down();
    static inline Vector3 Forward();
    static inline Vector3 Backward();

    static inline float Angle(Vector3 a, Vector3 b);
    static inline float NormalizeAngle(float angle);
    static inline Vector3 NormalizeAngles(Vector3 angles);
    static inline Vector3 ClampMagnitude(Vector3 vector, float maxLength);
    static inline float Component(Vector3 a, Vector3 b);
    static inline Vector3 Cross(Vector3 lhs, Vector3 rhs);
    static inline float Distance(Vector3 a, Vector3 b);
    static inline float Dot(Vector3 lhs, Vector3 rhs);
    static inline Vector3 FromSpherical(float rad, float theta, float phi);
    static inline Vector3 Lerp(Vector3 a, Vector3 b, float t);
    static inline Vector3 LerpUnclamped(Vector3 a, Vector3 b, float t);
    static inline float Magnitude(Vector3 v);
    static inline Vector3 Max(Vector3 a, Vector3 b);
    static inline Vector3 Min(Vector3 a, Vector3 b);
    static inline Vector3 MoveTowards(Vector3 current, Vector3 target, float maxDistanceDelta);
    static inline Vector3 Normalized(Vector3 v);
    static inline Vector3 Orthogonal(Vector3 v);
    static inline void OrthoNormalize(Vector3 &normal, Vector3 &tangent, Vector3 &binormal);
    static inline Vector3 Project(Vector3 a, Vector3 b);
    static inline Vector3 ProjectOnPlane(Vector3 vector, Vector3 planeNormal);
    static inline Vector3 Reflect(Vector3 vector, Vector3 planeNormal);
    static inline Vector3 Reject(Vector3 a, Vector3 b);
    static inline Vector3 RotateTowards(Vector3 current, Vector3 target, float maxRadiansDelta, float maxMagnitudeDelta);
    static inline Vector3 Scale(Vector3 a, Vector3 b);
    static inline Vector3 Slerp(Vector3 a, Vector3 b, float t);
    static inline Vector3 SlerpUnclamped(Vector3 a, Vector3 b, float t);
    static inline float SqrMagnitude(Vector3 v);
    static inline void ToSpherical(Vector3 vector, float &rad, float &theta, float &phi);

    inline struct Vector3& operator+=(const float rhs);
    inline struct Vector3& operator-=(const float rhs);
    inline struct Vector3& operator*=(const float rhs);
    inline struct Vector3& operator/=(const float rhs);
    inline struct Vector3& operator+=(const Vector3 rhs);
    inline struct Vector3& operator-=(const Vector3 rhs);
};

inline Vector3 operator-(Vector3 rhs);
inline Vector3 operator+(Vector3 lhs, const float rhs);
inline Vector3 operator-(Vector3 lhs, const float rhs);
inline Vector3 operator*(Vector3 lhs, const float rhs);
inline Vector3 operator/(Vector3 lhs, const float rhs);
inline Vector3 operator+(const float lhs, Vector3 rhs);
inline Vector3 operator-(const float lhs, Vector3 rhs);
inline Vector3 operator*(const float lhs, Vector3 rhs);
inline Vector3 operator/(const float lhs, Vector3 rhs);
inline Vector3 operator+(Vector3 lhs, const Vector3 rhs);
inline Vector3 operator-(Vector3 lhs, const Vector3 rhs);
inline bool operator==(const Vector3 lhs, const Vector3 rhs);
inline bool operator!=(const Vector3 lhs, const Vector3 rhs);

Vector3::Vector3() : x(0), y(0), z(0) {}
Vector3::Vector3(float data[]) : x(data[0]), y(data[1]), z(data[2]) {}
Vector3::Vector3(float value) : x(value), y(value), z(value) {}
Vector3::Vector3(float x, float y) : x(x), y(y), z(0) {}
Vector3::Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

Vector3 Vector3::zero() { return Vector3(0, 0, 0); }
Vector3 Vector3::One() { return Vector3(1, 1, 1); }
Vector3 Vector3::Right() { return Vector3(1, 0, 0); }
Vector3 Vector3::Left() { return Vector3(-1, 0, 0); }
Vector3 Vector3::Up() { return Vector3(0, 1, 0); }
Vector3 Vector3::Down() { return Vector3(0, -1, 0); }
Vector3 Vector3::Forward() { return Vector3(0, 0, 1); }
Vector3 Vector3::Backward() { return Vector3(0, 0, -1); }

float Vector3::Angle(Vector3 a, Vector3 b)
{
    float v = Dot(a, b) / (Magnitude(a) * Magnitude(b));
    v = fmax(v, -1.0);
    v = fmin(v, 1.0);
    return acos(v);
}

float Vector3::NormalizeAngle(float angle)
{
    while(angle > 360)
        angle -= 360;
    while(angle < 0)
        angle += 360;
    return angle;
}

Vector3 Vector3::NormalizeAngles(Vector3 angles)
{
    angles.x = NormalizeAngle(angles.x);
    angles.y = NormalizeAngle(angles.y);
    angles.z = NormalizeAngle(angles.z);
    return angles;
}

Vector3 Vector3::ClampMagnitude(Vector3 vector, float maxLength)
{
    float length = Magnitude(vector);
    if(length > maxLength) vector *= maxLength / length;
    return vector;
}

float Vector3::Component(Vector3 a, Vector3 b)
{
    return Dot(a, b) / Magnitude(b);
}

Vector3 Vector3::Cross(Vector3 lhs, Vector3 rhs)
{
    float x = lhs.y * rhs.z - lhs.z * rhs.y;
    float y = lhs.z * rhs.x - lhs.x * rhs.z;
    float z = lhs.x * rhs.y - lhs.y * rhs.x;
    return Vector3(x, y, z);
}

float Vector3::Distance(Vector3 a, Vector3 b)
{
    return Vector3::Magnitude(a - b);
}

float Vector3::Dot(Vector3 lhs, Vector3 rhs)
{
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

Vector3 Vector3::FromSpherical(float rad, float theta, float phi)
{
    Vector3 v;
    v.x = rad * sin(theta) * cos(phi);
    v.y = rad * sin(theta) * sin(phi);
    v.z = rad * cos(theta);
    return v;
}

Vector3 Vector3::Lerp(Vector3 a, Vector3 b, float t)
{
    if(t < 0) return a;
    else if(t > 1) return b;
    return LerpUnclamped(a, b, t);
}

Vector3 Vector3::LerpUnclamped(Vector3 a, Vector3 b, float t)
{
    return (b - a) * t + a;
}

float Vector3::Magnitude(Vector3 v)
{
    return sqrt(SqrMagnitude(v));
}

Vector3 Vector3::Max(Vector3 a, Vector3 b)
{
    float x = a.x > b.x ? a.x : b.x;
    float y = a.y > b.y ? a.y : b.y;
    float z = a.z > b.z ? a.z : b.z;
    return Vector3(x, y, z);
}

Vector3 Vector3::Min(Vector3 a, Vector3 b)
{
    float x = a.x > b.x ? b.x : a.x;
    float y = a.y > b.y ? b.y : a.y;
    float z = a.z > b.z ? b.z : a.z;
    return Vector3(x, y, z);
}

Vector3 Vector3::MoveTowards(Vector3 current, Vector3 target, float maxDistanceDelta)
{
    Vector3 d = target - current;
    float m = Magnitude(d);
    if(m < maxDistanceDelta || m == 0) return target;
    return current + (d * maxDistanceDelta / m);
}

Vector3 Vector3::Normalized(Vector3 v)
{
    float mag = Magnitude(v);
    if(mag == 0) return Vector3::zero();
    return v / mag;
}

Vector3 Vector3::Orthogonal(Vector3 v)
{
    return v.z < v.x ? Vector3(v.y, -v.x, 0) : Vector3(0, -v.z, v.y);
}

void Vector3::OrthoNormalize(Vector3 &normal, Vector3 &tangent, Vector3 &binormal)
{
    normal = Normalized(normal);
    tangent = ProjectOnPlane(tangent, normal);
    tangent = Normalized(tangent);
    binormal = ProjectOnPlane(binormal, tangent);
    binormal = ProjectOnPlane(binormal, normal);
    binormal = Normalized(binormal);
}

Vector3 Vector3::Project(Vector3 a, Vector3 b)
{
    float m = Magnitude(b);
    return Dot(a, b) / (m * m) * b;
}

Vector3 Vector3::ProjectOnPlane(Vector3 vector, Vector3 planeNormal)
{
    return Reject(vector, planeNormal);
}

Vector3 Vector3::Reflect(Vector3 vector, Vector3 planeNormal)
{
    return vector - 2 * Project(vector, planeNormal);
}

Vector3 Vector3::Reject(Vector3 a, Vector3 b)
{
    return a - Project(a, b);
}

Vector3 Vector3::RotateTowards(Vector3 current, Vector3 target, float maxRadiansDelta, float maxMagnitudeDelta)
{
    float magCur = Magnitude(current);
    float magTar = Magnitude(target);
    float newMag = magCur + maxMagnitudeDelta * ((magTar > magCur) - (magCur > magTar));
    newMag = fmin(newMag, fmax(magCur, magTar));
    newMag = fmax(newMag, fmin(magCur, magTar));

    float totalAngle = Angle(current, target) - maxRadiansDelta;
    if(totalAngle <= 0) return Normalized(target) * newMag;
    else if(totalAngle >= M_PI) return Normalized(-target) * newMag;

    Vector3 axis = Cross(current, target);
    float magAxis = Magnitude(axis);
    if(magAxis == 0) axis = Normalized(Cross(current, current + Vector3(3.95, 5.32, -4.24)));
    else axis /= magAxis;
    current = Normalized(current);
    Vector3 newVector = current * cos(maxRadiansDelta) + Cross(axis, current) * sin(maxRadiansDelta);
    return newVector * newMag;
}

Vector3 Vector3::Scale(Vector3 a, Vector3 b)
{
    return Vector3(a.x * b.x, a.y * b.y, a.z * b.z);
}

Vector3 Vector3::Slerp(Vector3 a, Vector3 b, float t)
{
    if(t < 0) return a;
    else if(t > 1) return b;
    return SlerpUnclamped(a, b, t);
}

Vector3 Vector3::SlerpUnclamped(Vector3 a, Vector3 b, float t)
{
    float magA = Magnitude(a);
    float magB = Magnitude(b);
    a /= magA;
    b /= magB;
    float dot = Dot(a, b);
    dot = fmax(dot, -1.0);
    dot = fmin(dot, 1.0);
    float theta = acos(dot) * t;
    Vector3 relativeVec = Normalized(b - a * dot);
    Vector3 newVec = a * cos(theta) + relativeVec * sin(theta);
    return newVec * (magA + (magB - magA) * t);
}

float Vector3::SqrMagnitude(Vector3 v)
{
    return v.x * v.x + v.y * v.y + v.z * v.z;
}

void Vector3::ToSpherical(Vector3 vector, float &rad, float &theta, float &phi)
{
    rad = Magnitude(vector);
    float v = vector.z / rad;
    v = fmax(v, -1.0);
    v = fmin(v, 1.0);
    theta = acos(v);
    phi = atan2(vector.y, vector.x);
}

struct Vector3& Vector3::operator+=(const float rhs)
{
    x += rhs;
    y += rhs;
    z += rhs;
    return *this;
}

struct Vector3& Vector3::operator-=(const float rhs)
{
    x -= rhs;
    y -= rhs;
    z -= rhs;
    return *this;
}

struct Vector3& Vector3::operator*=(const float rhs)
{
    x *= rhs;
    y *= rhs;
    z *= rhs;
    return *this;
}

struct Vector3& Vector3::operator/=(const float rhs)
{
    x /= rhs;
    y /= rhs;
    z /= rhs;
    return *this;
}

struct Vector3& Vector3::operator+=(const Vector3 rhs)
{
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
}

struct Vector3& Vector3::operator-=(const Vector3 rhs)
{
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
}

Vector3 operator-(Vector3 rhs) { return rhs * -1; }
Vector3 operator+(Vector3 lhs, const float rhs) { return lhs += rhs; }
Vector3 operator-(Vector3 lhs, const float rhs) { return lhs -= rhs; }
Vector3 operator*(Vector3 lhs, const float rhs) { return lhs *= rhs; }
Vector3 operator/(Vector3 lhs, const float rhs) { return lhs /= rhs; }
Vector3 operator+(const float lhs, Vector3 rhs) { return rhs += lhs; }
Vector3 operator-(const float lhs, Vector3 rhs) { return rhs -= lhs; }
Vector3 operator*(const float lhs, Vector3 rhs) { return rhs *= lhs; }
Vector3 operator/(const float lhs, Vector3 rhs) { return rhs /= lhs; }
Vector3 operator+(Vector3 lhs, const Vector3 rhs) { return lhs += rhs; }
Vector3 operator-(Vector3 lhs, const Vector3 rhs) { return lhs -= rhs; }
bool operator==(const Vector3 lhs, const Vector3 rhs) { return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z; }
bool operator!=(const Vector3 lhs, const Vector3 rhs) { return !(lhs == rhs); }

// Forward declarations for PID functions
mach_port_t get_task_for_PID(pid_t pid);
pid_t get_pid_by_name(const char *keyword);
task_t get_task_by_pid(pid_t pid);
mach_vm_address_t get_image_base_address(mach_port_t task, const char *image_name);

template<typename T>
T Read(uintptr_t address, task_t task);

// Forward declarations for Unity API functions
Vector3 get_position_by_transform(mach_vm_address_t mach_transform_ptr, task_t task);
inline float Dot(const Vector3 &Vec1, const Vector3 &Vec2);
Vector3 WorldToScreen(Vector3 object, mach_vm_address_t camera_ptr, CGFloat ScreenWidth, CGFloat ScreenHeight, task_t task);

// PID function implementations
mach_port_t get_task_for_PID(pid_t pid)
{
    mach_port_t task;
    kern_return_t kr = task_for_pid(mach_task_self(), pid, &task);
    if (kr == KERN_SUCCESS)
    {
        return task;
    }
    
    return MACH_PORT_NULL;
}

pid_t get_pid_by_name(const char *keyword)
{
    int count = proc_listallpids(NULL, 0);
    pid_t *pids = (pid_t*)malloc(count * sizeof(pid_t));
    if (!pids) return -1;
    proc_listallpids(pids, count * sizeof(pid_t));
    
    for (int i = 0; i < count; i++)
    {
        char name[1000];
        proc_name(pids[i], name, sizeof(name));
        if (strstr(name, keyword) != NULL)
        {
            pid_t result = pids[i];
            free(pids);
            return result;
        }
    }
    
    free(pids);
    return -1;
}

task_t get_task_by_pid(pid_t pid)
{
    task_port_t psDefault;
    task_port_t psDefault_control;

    task_array_t tasks;
    mach_msg_type_number_t numTasks;
    kern_return_t kr;

    host_t self_host = mach_host_self();
    kr = processor_set_default(self_host, &psDefault);
    if (kr != KERN_SUCCESS)
    {
        fprintf(stderr, "Error in processor_set_default: %x\n", kr);
        return MACH_PORT_NULL;
    }

    kr = host_processor_set_priv(self_host, psDefault, &psDefault_control);
    if (kr != KERN_SUCCESS)
    {
        fprintf(stderr, "Error in host_processor_set_priv: %x\n", kr);
        return MACH_PORT_NULL;
    }

    kr = processor_set_tasks(psDefault_control, &tasks, &numTasks);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "Error in processor_set_tasks: %x\n", kr);
        return MACH_PORT_NULL;
    }

    for (int i = 0; i < numTasks; i++)
    {
        int task_pid;
        kr = pid_for_task(tasks[i], &task_pid);
        if (kr != KERN_SUCCESS) {
            continue;
        }

        if (task_pid == pid) return tasks[i];
    }

    return MACH_PORT_NULL;
}

mach_vm_address_t get_image_base_address(mach_port_t task, const char *image_name)
{
    task_dyld_info_data_t dyld_info;
    mach_msg_type_number_t count = TASK_DYLD_INFO_COUNT;
    kern_return_t kr = task_info(task, TASK_DYLD_INFO, (task_info_t)&dyld_info, &count);
    if (kr != KERN_SUCCESS)
    {
        fprintf(stderr, "task_info failed: %s\n", mach_error_string(kr));
        return 0;
    }

    struct dyld_all_image_infos64 {
        uint32_t version;
        uint32_t infoArrayCount;
        mach_vm_address_t infoArray;
        dyld_image_notifier  notification;
        bool                 processDetachedFromSharedRegion;
        bool libSystemInitialized;
        mach_vm_address_t            dyldImageLoadAddress;
        mach_vm_address_t            jitInfo;
        mach_vm_address_t            dyldVersion;
        mach_vm_address_t            errorMessage;
        uint64_t                    terminationFlags;
        mach_vm_address_t            coreSymbolicationShmPage;
        uint64_t                    systemOrderFlag;
        uint64_t                    uuidArrayCount;
        mach_vm_address_t            uuidArray;
        mach_vm_address_t            dyldAllImageInfosAddress;
        uint64_t                    initialImageCount;
        uint64_t                    errorKind;
        mach_vm_address_t            errorClientOfDylibPath;
        mach_vm_address_t            errorTargetDylibPath;
        mach_vm_address_t            errorSymbol;
        uint64_t                    sharedCacheSlide;
    };

    struct dyld_image_info64 {
        mach_vm_address_t    imageLoadAddress;
        mach_vm_address_t    imageFilePath;
        mach_vm_size_t       imageFileModDate;
    };

    struct dyld_all_image_infos64 infos;
    vm_size_t size = sizeof(infos);
    mach_msg_type_number_t read_size = 0;
    vm_offset_t read_mem = 0;

    kr = vm_read(task, (vm_address_t)dyld_info.all_image_info_addr, size, &read_mem, &read_size);
    if (kr != KERN_SUCCESS || read_size < sizeof(infos))
    {
        fprintf(stderr, "vm_read for dyld_all_image_infos64 failed: %s\n", mach_error_string(kr));
        return 0;
    }
    memcpy(&infos, (void *)read_mem, sizeof(infos));
    vm_deallocate(mach_task_self(), read_mem, read_size);

    uint32_t image_count = infos.infoArrayCount;
    mach_vm_address_t info_array_addr = infos.infoArray;
    vm_size_t image_info_size = image_count * sizeof(struct dyld_image_info64);
    struct dyld_image_info64 *image_infos = (struct dyld_image_info64 *)malloc(image_info_size);
    if (!image_infos) return 0;

    read_mem = 0;
    read_size = 0;
    kr = vm_read(task, (vm_address_t)info_array_addr, image_info_size, &read_mem, &read_size);
    if (kr != KERN_SUCCESS || read_size < image_info_size)
    {
        fprintf(stderr, "vm_read for image infos failed: %s\n", mach_error_string(kr));
        free(image_infos);
        return 0;
    }
    memcpy(image_infos, (void *)read_mem, image_info_size);
    vm_deallocate(mach_task_self(), read_mem, read_size);

    for (uint32_t i = 0; i < image_count; ++i)
    {
        char path_buffer[PATH_MAX] = {0};
        read_mem = 0;
        read_size = 0;
        kr = vm_read(task, (vm_address_t)image_infos[i].imageFilePath, PATH_MAX, &read_mem, &read_size);
        if (kr == KERN_SUCCESS)
        {
            size_t to_copy = read_size > PATH_MAX ? PATH_MAX : read_size;
            memcpy(path_buffer, (void *)read_mem, to_copy);
            vm_deallocate(mach_task_self(), read_mem, read_size);
        }

        if (kr == KERN_SUCCESS && strstr(path_buffer, image_name))
        {
            mach_vm_address_t base = image_infos[i].imageLoadAddress;
            free(image_infos);
            return base;
        }
    }

    free(image_infos);
    return 0;
}

struct ESPBox {
    Vector3 pos;
    CGFloat width;
    CGFloat height;
};

@interface ESP_View : UIView

- (instancetype)initWithFrame:(CGRect)frame;
- (void)setBoxes:(NSArray<NSValue *> *)boxes;
- (void)updateBoxes;
- (void)update_data;
@end
