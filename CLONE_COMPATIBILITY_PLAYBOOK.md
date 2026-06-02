# Playbook de Compatibilidad para Clones de Prusa Mini (Para Agentes de IA)

Este documento es una guía auto-contenida (runbook/playbook) diseñada para que cualquier **Agente de IA o desarrollador** aplique de forma automática los parches de compatibilidad para tarjetas clones de Prusa Mini en versiones futuras de este repositorio.

> [!IMPORTANT]
> Las actualizaciones del repositorio original (`upstream`) pueden alterar los números de línea o incluso mover funciones a otros archivos. Este playbook utiliza **anclas de contexto** (patrones de código de búsqueda única) en lugar de números de línea rígidos para garantizar que los cambios se puedan aplicar en cualquier versión.

---

## 📋 Resumen de Parches Críticos

Para que el firmware `Buddy` funcione en clones de Prusa Mini (que usan memorias flash SPI no originales, no tienen número de serie grabado en la OTP original, etc.), se deben aplicar **4 parches**:

1. **Bypass del ID del Fabricante de Memoria Flash** (Evita BSOD por flash SPI alternativa).
2. **Generación de Número de Serie Sintético** (Evita BSOD por falta de serie OTP).
3. **Velocidad del Ventilador de Heatbreak al 100%** (Previene atascos por calor en hotends clonados).
4. **Relajación de Tolerancias de Autodiagnóstico (Selftest)** (Evita fallos falsos en autodiagnóstico).

---

## 🛠️ Paso a Paso para la Aplicación del Parche

Cualquier agente que lea este archivo debe seguir los siguientes pasos secuenciales:

### Paso 1: Bypass de Validación de Memoria Flash (W25X)

*   **Archivo Objetivo:** `src/common/w25x.cpp`
*   **Ancla de Búsqueda:** Busca la función `mfrid_devid` usando el patrón: `int mfrid_devid(uint8_t *devid)`
*   **Modificación:**
    Reemplaza la validación del fabricante para que solo verifique que el `device_id` sea válido, ignorando el `w25x_mfrid == MFRID`.

#### Diffs de Reemplazo:
```diff
<<<<
    if (devid) {
        *devid = w25x_devid;
    }
    return ((w25x_mfrid == MFRID) && ((w25x_devid == DEVID) || (w25x_devid == DEVID_NEW)));
}
====
    if (devid) {
        *devid = w25x_devid;
    }
    return (((w25x_devid == DEVID) || (w25x_devid == DEVID_NEW)));
}
>>>>
```

---

### Paso 2: Número de Serie Sintético (Evitar BSOD OTP)

*   **Archivo Objetivo:** `src/connect/printer_common.cpp`
*   **Anclas de Búsqueda:**
    1. Busca la inclusión de cabeceras en el tope del archivo.
    2. Busca la función `void init_info(Printer::PrinterInfo &info)`.
*   **Modificación:**
    *   Importa `<mbedtls/sha256.h>` para poder hashear el UUID.
    *   Inserta las funciones auxiliares `serial_valid` y `synthetic_serial` antes de `load_eeprom_config` o `init_info`.
    *   Modifica `init_info` para que valide el número de serie de la OTP y, si es inválido o está vacío, use el sintético en lugar de invocar `bsod`.

#### Diffs de Reemplazo:

**1. Cabeceras:**
```diff
<<<<
#include <support_utils.h>
#include <version/version.hpp>

namespace connect_client {
====
#include <support_utils.h>
#include <version/version.hpp>
#include <mbedtls/sha256.h>

namespace connect_client {

namespace {
    // Valida que el número de serie tenga caracteres legibles
    bool serial_valid(const char *sn) {
        for (const char *c = sn; *c; c++) {
            if (!isprint(*c)) {
                return false;
            }
        }
        return true;
    }

    // Genera un número de serie determinista y legible usando el UUID del microcontrolador
    uint8_t synthetic_serial(serial_nr_t *sn) {
        memset(sn->begin(), 0, sn->size());
        strlcpy(sn->begin(), "DEVX", sn->size());
        static const char salt[] = "Nj20je98gje";
        mbedtls_sha256_context ctx;
        mbedtls_sha256_init(&ctx);
        mbedtls_sha256_starts_ret(&ctx, false);
        mbedtls_sha256_update_ret(&ctx, (const uint8_t *)salt, sizeof salt);
        uint32_t timestamp = otp_get_timestamp();
        mbedtls_sha256_update_ret(&ctx, (const uint8_t *)&timestamp, sizeof timestamp);
        mbedtls_sha256_update_ret(&ctx, otp_get_STM32_UUID()->uuid, sizeof(otp_get_STM32_UUID()->uuid));
        mbedtls_sha256_update_ret(&ctx, (const uint8_t *)salt, sizeof salt);
        uint8_t hash[32];
        mbedtls_sha256_finish_ret(&ctx, hash);
        mbedtls_sha256_free(&ctx);
        const size_t offset = 4;
        for (size_t i = 0; i < 15; i++) {
            (*sn)[i + offset] = 'a' + (hash[i] & 0x0f);
        }
        return 20;
    }
} // namespace
>>>>
```

**2. En la función `init_info`:**
```diff
<<<<
void init_info(Printer::PrinterInfo &info) {
    info.firmware_version = version::project_version_full;
    info.appendix = appendix_exist();
    if (otp_get_serial_nr(info.serial_number) == 0) {
        bsod("otp_get_serial_nr");
    }
    printerHash(info.fingerprint, sizeof(info.fingerprint) - 1, false);
====
void init_info(Printer::PrinterInfo &info) {
    info.firmware_version = version::project_version_full;
    info.appendix = appendix_exist();
    otp_get_serial_nr(info.serial_number);
    if (!serial_valid(info.serial_number.begin())) {
        synthetic_serial(&info.serial_number);
    }
    printerHash(info.fingerprint, sizeof(info.fingerprint) - 1, false);
>>>>
```

---

### Paso 3: Aumentar la velocidad del ventilador del Heatbreak

*   **Archivo Objetivo:** `src/common/hwio_buddy_2209_02.cpp`
*   **Ancla de Búsqueda:** Busca la línea `Fans::heat_break(0).set_pwm(ulVal ? 80 : 0);` dentro del bloque de directivas `#elif (PRINTER_IS_PRUSA_XL() || PRINTER_IS_PRUSA_MINI())`.
*   **Modificación:** Cambiar el valor PWM de `80` a `255` (100% velocidad) para mayor enfriamiento en clones.

#### Diffs de Reemplazo:
```diff
<<<<
#elif (PRINTER_IS_PRUSA_XL() || PRINTER_IS_PRUSA_MINI())
        Fans::heat_break(0).set_pwm(ulVal ? 80 : 0);
#else
====
#elif (PRINTER_IS_PRUSA_XL() || PRINTER_IS_PRUSA_MINI())
        Fans::heat_break(0).set_pwm(ulVal ? 255 : 0);
#else
>>>>
```

---

### Paso 4: Relajar las Tolerancias de Autodiagnóstico (Selftest)

*   **Archivo Objetivo:** `src/common/selftest/selftest_MINI.cpp`
*   **Anclas de Búsqueda:**
    1. `static const AxisConfig_t Config_ZAxis`
    2. `static const HeaterConfig_t Config_HeaterNozzle`
    3. `static const HeaterConfig_t Config_HeaterBed`
*   **Modificación:** Incrementar la tolerancia del eje Z y relajar las temperaturas mínimas de calentamiento.

#### Diffs de Reemplazo:

**1. Eje Z:**
```diff
<<<<
    .fr_table_fw = Zfr_table_fw,
    .fr_table_bw = Zfr_table_bw,
    .length_min = get_z_max_pos_mm() - 4,
    .length_max = get_z_max_pos_mm() + 6,
====
    .fr_table_fw = Zfr_table_fw,
    .fr_table_bw = Zfr_table_bw,
    .length_min = get_z_max_pos_mm() - 5,
    .length_max = get_z_max_pos_mm() + 6,
>>>>
```

**2. Calentador de Boquilla:**
```diff
<<<<
        .start_temp = 40,
        .undercool_temp = 37,
        .target_temp = 290,
        .heat_min_temp = 130,
        .heat_max_temp = 190,
====
        .start_temp = 40,
        .undercool_temp = 37,
        .target_temp = 290,
        .heat_min_temp = 120,
        .heat_max_temp = 190,
>>>>
```

**3. Calentador de Cama:**
```diff
<<<<
    .start_temp = 40,
    .undercool_temp = 39,
    .target_temp = 110,
    .heat_min_temp = 50,
    .heat_max_temp = 65,
====
    .start_temp = 40,
    .undercool_temp = 39,
    .target_temp = 110,
    .heat_min_temp = 45,
    .heat_max_temp = 65,
>>>>
```

---

## 🚀 Paso 5: Compilación y Verificación

Una vez aplicados los parches anteriores, el agente debe ejecutar el comando de compilación local del firmware de la MINI para asegurarse de que todo compila correctamente:

```bash
python utils/build.py --printer mini
```

Si la compilación se completa sin errores, los binarios se generarán en `./build/products`.
