#pragma once

#define ph_new(type) new type
#define ph_new_array(type, size) new type[size]
#define ph_delete(ptr) delete ptr
#define ph_delete_array(ptr) delete[] ptr
