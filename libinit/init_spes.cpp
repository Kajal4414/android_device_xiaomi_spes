/*
 * Copyright (C) 2024 The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstdlib>
#include <string.h>

#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>
#include <sys/sysinfo.h>
#include <android-base/properties.h>

#include "property_service.h"
#include "vendor_init.h"

using android::base::GetProperty;
using std::string;

// Function to override a system property
void property_override(string prop, string value) {
    auto pi = (prop_info*) __system_property_find(prop.c_str());

    if (pi != nullptr) {
        __system_property_update(pi, value.c_str(), value.size());
    } else {
        __system_property_add(prop.c_str(), prop.size(), value.c_str(), value.size());
    }
}

// Function to load Dalvik properties based on RAM size
void load_dalvik_properties() {
    struct sysinfo sys;
    sysinfo(&sys);

    // Check if total RAM is less than 4GB
    if (sys.totalram < 4096ull * 1024 * 1024) {
        // Set Dalvik heap properties
        property_override("dalvik.vm.heapstartsize", "8m");
        property_override("dalvik.vm.heapgrowthlimit", "256m");
        property_override("dalvik.vm.heapmaxfree", "16m");
        property_override("dalvik.vm.heaptargetutilization", "0.75");
        property_override("dalvik.vm.heapsize", "512m");
        property_override("dalvik.vm.heapminfree", "512k");

        // Set LMKD properties
        property_override("ro.lmk.psi_complete_stall_ms", "600");
        property_override("ro.lmk.psi_partial_stall_ms", "130");
        property_override("ro.lmk.swap_free_low_percentage", "20");
        property_override("ro.lmk.thrashing_limit", "55");
        property_override("ro.lmk.thrashing_limit_decay", "37");
    }
}

// Function to set read-only build properties
void set_ro_build_prop(const string &source, const string &prop, const string &value, bool product = false) {
    string prop_name;

    if (product) {
        prop_name = "ro.product." + source + prop;
    } else {
        prop_name = "ro." + source + "build." + prop;
    }

    property_override(prop_name.c_str(), value.c_str());
}

// Function to set device-specific properties
void set_device_props(const string model, const string name, const string marketname, const string mod_device) {
    // List of partitions to override props
    string source_partitions[] = { "", "bootimage.", "product.", "system.", "system_ext.", "vendor." };

    for (const string &source : source_partitions) {
        set_ro_build_prop(source, "model", model, true);
        set_ro_build_prop(source, "name", name, true);
        set_ro_build_prop(source, "marketname", marketname, true);
    }

    property_override("ro.product.mod_device", mod_device.c_str());
    property_override("bluetooth.device.default_name", marketname.c_str());
    property_override("vendor.usb.product_string", marketname.c_str());
}

// Main function to load vendor properties
void vendor_load_properties() {
    // Detect device and configure properties
    if (GetProperty("ro.boot.hwname", "") == "spesn") { // Redmi Note 11 NFC
        set_device_props("2201117TY", "spesn_global", "Redmi Note 11 NFC", "spesn_global");
    } else { // Redmi Note 11
        string region = GetProperty("ro.boot.hwc", "");
        if (region == "IN") { // Redmi Note 11 India
            set_device_props("2201117TI", "spes_in", "Redmi Note 11", "spes_in_global");
        } else { // Redmi Note 11 Global
            set_device_props("2201117TG", "spes_global", "Redmi Note 11", "spes_global");
        }
    }

    // Set hardware revision
    property_override("ro.boot.hardware.revision", GetProperty("ro.boot.hwversion", "").c_str());

    // Set Dalvik heap configuration
    load_dalvik_properties();
}
