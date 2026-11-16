/**
 * ============================================================================
 * BÀI TẬP LUYỆN CODE EMBEDDED C - NGÀY 02 (Thời lượng: 45 - 60 phút)
 * Chủ đề: Đọc Bit, Đảo Bit (Toggle), Cụm Đa Bit (Bit-field) & Ghép Byte Cảm Biến
 * Tài liệu tham chiếu:
 *   - Chuẩn ngôn ngữ C (stdint/bitwise): https://cppreference.net/c/language.html
 *   - Quy trình lưu code Git hàng ngày:  https://git-scm.com/cheat-sheet
 * ============================================================================
 * HƯỚNG DẪN LÀM BÀI SÁNG MAI:
 * 1. Đọc kỹ sơ đồ thanh ghi 32-bit `cfg_reg` bên dưới.
 * 2. Tìm 4 vị trí có chữ `// TODO:` ở Nhiệm vụ 1, 2, 3, 4 và viết code C của bạn vào đó.
 * 3. Biên dịch và chạy chương trình. Hàm `main()` ở cuối file là hệ thống CHẤM ĐIỂM
 *    TỰ ĐỘNG: Khi bạn code đúng cả 4 hàm, màn hình sẽ hiện đủ 4 dòng [PASS]!
 * 4. Sau khi đạt 4/4 [PASS], mở Terminal và đẩy bài lên GitHub:
 *      git add day02_register_practice.c
 *      git commit -m "Day 02: Hoan thanh 4 bai tap thao tac thanh ghi nang cao"
 *      git push
 * ============================================================================
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/* ============================================================================
 * SƠ ĐỒ THANH GHI CẤU HÌNH `cfg_reg` (32-bit, ta tập trung vào 8 bit thấp):
 *
 *   Bit 7       Bit 6..4      Bit 3..2          Bit 1           Bit 0
 * +-----------+------------+---------------+----------------+---------------+
 * | ERROR_FLG |  Dự phòng  |  GAIN_SELECT  | LOW_POWER_MODE | SENSOR_ENABLE |
 * +-----------+------------+---------------+----------------+---------------+
 *   (1 bit)      (3 bit)       (2 bit)          (1 bit)         (1 bit)
 * ============================================================================ */

#define CFG_ENABLE_BIT      (1U << 0)           // Bit 0: 1 = Bật cảm biến, 0 = Tắt
#define CFG_LOWPOWER_BIT    (1U << 1)           // Bit 1: 1 = Ngủ tiết kiệm điện, 0 = Hoạt động
#define CFG_GAIN_POS        (2U)                // Cụm 2 bit Gain bắt đầu từ vị trí Bit 2
#define CFG_GAIN_MASK       (3U << CFG_GAIN_POS)// Mặt nạ 2 bit ở vị trí Bit 2..3 (0b00001100 = 0x0C)
#define CFG_ERROR_FLAG      (1U << 7)           // Bit 7: 1 = Có lỗi vượt ngưỡng, 0 = Bình thường

typedef struct {
    uint8_t  device_id;
    uint32_t cfg_reg;       // Thanh ghi cấu hình 32-bit
    int16_t  last_sample;   // Giá trị mẫu cảm biến 16-bit gần nhất
} SmartSensor_t;


/* ============================================================================
 * NHIỆM VỤ 1 (10 phút): KIỂM TRA TRẠNG THÁI 1 BIT (READ BIT)
 * Yêu cầu:
 *   - Nếu con trỏ `dev == NULL`, trả về `false`.
 *   - Kiểm tra xem bit `CFG_ENABLE_BIT` (Bit 0) trong `dev->cfg_reg` đang là 1 hay 0.
 *   - Trả về `true` nếu bit đó bằng 1, trả về `false` nếu bit đó bằng 0.
 * Gợi ý:
 *   - Dùng phép AND (`&`) giữa `dev->cfg_reg` và `CFG_ENABLE_BIT`.
 *   - Tuyệt đối KHÔNG dùng phép gán `&=`, vì hàm này chỉ đọc chứ không sửa thanh ghi!
 * ============================================================================ */
bool Sensor_IsEnabled(const SmartSensor_t *dev) {
    if (dev == NULL) {
        return false;
    }
    if( ((dev->cfg_reg & ( 1U << 0)) != 0 )  ){
        return true;
    } else {
        return false ;
    }

    // TODO: Viết code kiểm tra bit CFG_ENABLE_BIT tại đây và return kết quả
    return false;
}


/* ============================================================================
 * NHIỆM VỤ 2 (10 phút): ĐẢO TRẠNG THÁI BIT (TOGGLE BIT)
 * Yêu cầu:
 *   - Kiểm tra `dev == NULL` thì thoát hàm (`return;`).
 *   - Đảo trạng thái của bit `CFG_LOWPOWER_BIT` (Bit 1) trong `dev->cfg_reg`:
 *     + Nếu Bit 1 đang là 0 thì bật lên 1.
 *     + Nếu Bit 1 đang là 1 thì tắt về 0.
 *     + Giữ nguyên tất cả các bit khác!
 * Gợi ý:
 *   - Sử dụng toán tử XOR gán (`^=`) với mặt nạ `CFG_LOWPOWER_BIT` (chỉ mất đúng 1 dòng lệnh).
 * ============================================================================ */
void Sensor_ToggleLowPower(SmartSensor_t *dev) {
    if (dev == NULL) {
        return;
    }
    if( (dev->cfg_reg ^= (1U << 1) )  ){
        return;
    } 

    // TODO: Viết 1 dòng lệnh đảo bit CFG_LOWPOWER_BIT tại đây

}


/* ============================================================================
 * NHIỆM VỤ 3 (15 phút): GHI CỤM 2 BIT LIÊN TIẾP (MULTI-BIT FIELD)
 * Yêu cầu:
 *   - Cảm biến có 4 mức khuếch đại (Gain): 0 (1x), 1 (2x), 2 (4x), 3 (8x) nằm ở Bit 2 và Bit 3.
 *   - Kiểm tra nếu `dev == NULL` hoặc `gain_level > 3` thì thoát hàm ngay (`return;`).
 *   - Bước A: Xóa sạch 2 bit cũ tại vị trí Bit 2..3 về 0 (dùng `&= ~CFG_GAIN_MASK`).
 *   - Bước B: Dịch giá trị `gain_level` sang trái `CFG_GAIN_POS` (2 bit) rồi OR (`|=`) vào `dev->cfg_reg`.
 * ============================================================================ */
void Sensor_SetGain(SmartSensor_t *dev, uint8_t gain_level) {
    if ((dev == NULL) || (gain_level > 3U)) {
        return;
    }

    // TODO: Thực hiện Bước A (xóa 2 bit cũ) và Bước B (ghi 2 bit mới) vào dev->cfg_reg

    dev->cfg_reg &= ~ (1U << 2);
    dev->cfg_reg &= ~ (1U << 3);
    gain_level <<= 2;
    dev->cfg_reg |= gain_level;
}


/* ============================================================================
 * NHIỆM VỤ 4 (15 phút): GHÉP 2 BYTE DỮ LIỆU & BẬT CỜ BÁO LỖI
 * Yêu cầu:
 *   - Nếu `dev == NULL`, trả về 0.
 *   - Bước A: Ghép `msb` (8 bit cao) và `lsb` (8 bit thấp) thành số nguyên 16-bit có dấu (`int16_t`)
 *     rồi lưu vào `dev->last_sample`.
 *   - Bước B: So sánh `dev->last_sample` với ngưỡng `max_limit`:
 *     + Nếu `dev->last_sample > max_limit`: Bật bit `CFG_ERROR_FLAG` (Bit 7) trong `dev->cfg_reg` lên 1.
 *     + Ngược lại: Xóa bit `CFG_ERROR_FLAG` (Bit 7) trong `dev->cfg_reg` về 0.
 *   - Bước C: Trả về giá trị `dev->last_sample`.
 * ============================================================================ */
int16_t Sensor_ReadAndCheck(SmartSensor_t *dev, uint8_t msb, uint8_t lsb, int16_t max_limit) {
    if (dev == NULL) {
        return 0;
    }

    // TODO: Ghép byte vào dev->last_sample, kiểm tra max_limit để bật/tắt CFG_ERROR_FLAG, và return
    int16_t ketqua = ( msb << 8) | lsb;
    dev->last_sample = ketqua;
    if( dev->last_sample > max_limit){
        dev->cfg_reg |= ( 1U << 7);
    } else {
        dev->cfg_reg &= ~ ( 1U << 7);
    }
    return dev->last_sample;
}


/* ============================================================================
 * HỆ THỐNG CHẤM ĐIỂM TỰ ĐỘNG (KHÔNG CẦN SỬA HÀM MAIN NÀY)
 * Chạy chương trình để kiểm tra xem bạn đạt bao nhiêu [PASS] trên tổng số 4 bài!
 * ============================================================================ */
int main(void) {
    uint8_t pass_count = 0;
    SmartSensor_t sensor = {
        .device_id   = 101,
        .cfg_reg     = CFG_ENABLE_BIT, // Khởi tạo: Bit 0 = 1 (0x00000001)
        .last_sample = 0
    };

    printf("=== BAT DAU CHAM DIEM BAI TAP NGAY 02 ===\n\n");

    /* --- Chấm Nhiệm vụ 1 --- */
    bool check1_on = Sensor_IsEnabled(&sensor);
    sensor.cfg_reg &= ~CFG_ENABLE_BIT; // Thử tắt Bit 0
    bool check1_off = Sensor_IsEnabled(&sensor);
    sensor.cfg_reg |= CFG_ENABLE_BIT;  // Bật lại Bit 0

    if ((check1_on == true) && (check1_off == false)) {
        printf("[PASS] Nhiem vu 1: Ham Sensor_IsEnabled doc bit chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Nhiem vu 1: Khi Bit0=1 tra ve %d (ky vong 1), khi Bit0=0 tra ve %d (ky vong 0)\n",
               check1_on, check1_off);
    }

    /* --- Chấm Nhiệm vụ 2 --- */
    Sensor_ToggleLowPower(&sensor); // Lần 1: Bit 1 từ 0 -> 1 (cfg_reg kỳ vọng: 0x03)
    uint32_t after_toggle_1 = sensor.cfg_reg;
    Sensor_ToggleLowPower(&sensor); // Lần 2: Bit 1 từ 1 -> 0 (cfg_reg kỳ vọng: 0x01)
    uint32_t after_toggle_2 = sensor.cfg_reg;

    if ((after_toggle_1 == 0x03U) && (after_toggle_2 == 0x01U)) {
        printf("[PASS] Nhiem vu 2: Ham Sensor_ToggleLowPower dao bit chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Nhiem vu 2: Sau lan 1 cfg_reg=0x%02X (ky vong 0x03), lan 2 cfg_reg=0x%02X (ky vong 0x01)\n",
               after_toggle_1, after_toggle_2);
    }

    /* --- Chấm Nhiệm vụ 3 --- */
    Sensor_SetGain(&sensor, 3U); // Đặt Gain = 3 (0b11 ở Bit 3..2) -> cfg_reg = 0x0D (0b00001101)
    uint32_t reg_gain3 = sensor.cfg_reg;
    Sensor_SetGain(&sensor, 1U); // Đổi Gain = 1 (0b01 ở Bit 3..2) -> cfg_reg = 0x05 (0b00000101)
    uint32_t reg_gain1 = sensor.cfg_reg;

    if ((reg_gain3 == 0x0DU) && (reg_gain1 == 0x05U)) {
        printf("[PASS] Nhiem vu 3: Ham Sensor_SetGain xoa va ghi cum 2 bit chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Nhiem vu 3: Khi Gain=3 cfg_reg=0x%02X (ky vong 0x0D), khi Gain=1 cfg_reg=0x%02X (ky vong 0x05)\n",
               reg_gain3, reg_gain1);
    }

    /* --- Chấm Nhiệm vụ 4 --- */
    // Mẫu 1: 0x03E8 = 1000 (vượt ngưỡng 800 -> Bit 7 phải bật lên 1 -> 0x85)
    int16_t val1 = Sensor_ReadAndCheck(&sensor, 0x03, 0xE8, 800);
    uint32_t reg_err = sensor.cfg_reg;
    // Mẫu 2: 0x01F4 = 500 (nhỏ hơn ngưỡng 800 -> Bit 7 phải xóa về 0 -> 0x05)
    int16_t val2 = Sensor_ReadAndCheck(&sensor, 0x01, 0xF4, 800);
    uint32_t reg_ok = sensor.cfg_reg;

    if ((val1 == 1000) && (reg_err == 0x85U) && (val2 == 500) && (reg_ok == 0x05U)) {
        printf("[PASS] Nhiem vu 4: Ham Sensor_ReadAndCheck ghep byte va bat/tat co loi chinh xac!\n");
        pass_count++;
    } else {
        printf("[FAIL] Nhiem vu 4: val1=%d, reg_err=0x%02X (ky vong 1000, 0x85) | val2=%d, reg_ok=0x%02X (ky vong 500, 0x05)\n",
               val1, reg_err, val2, reg_ok);
    }
}