# Hệ thống quản lý bãi đỗ xe sinh viên

Chương trình C++17 console dành cho bài tập nhóm môn Lập trình hướng đối tượng. Quản lý xe máy và xe máy điện, mô phỏng quẹt thẻ bằng nhập mã thẻ, nhập biển số thủ công, lưu dữ liệu vào tệp văn bản. Chương trình chạy trên một máy, một phiên sử dụng tệp tại một thời điểm.

## 1. Môi trường và build

Windows có PowerShell và `g++` hỗ trợ C++17, chẳng hạn **MSYS2 UCRT64**. Thêm `C:\msys64\ucrt64\bin` vào PATH để tìm trình biên dịch và các DLL runtime của MinGW. Code nghiệp vụ không cần thư viện ngoài. Compiler đã kiểm tra: GCC 16.1.0.

Mở PowerShell trong thư mục `source_code`:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\build.ps1
.\build\parking.exe
```

Từ thư mục gốc dự án có thể dùng:

```powershell
powershell -ExecutionPolicy Bypass -File .\source_code\scripts\build.ps1
.\source_code\build\parking.exe
```

Script dùng C++17, `-Wall -Wextra -Wpedantic`, UTF-8 và đường dẫn tương đối khi gọi MinGW để chạy được trong thư mục cha có dấu tiếng Việt. Nếu terminal hiển thị tiếng Việt chưa đúng, sử dụng Windows Terminal với font Consolas. Chương trình tự đặt bảng mã console UTF-8 trên Windows.

Trên Linux/macOS có `g++`, có thể biên dịch thủ công từ `source_code`:

```bash
mkdir -p build
g++ -std=c++17 -Wall -Wextra -Wpedantic -Iinclude src/main.cpp src/models/*.cpp src/pricing/*.cpp src/services/*.cpp src/storage/*.cpp src/ui/*.cpp -o build/parking
./build/parking --demo
```

Phần nghiệp vụ dùng thư viện chuẩn; các lệnh PowerShell và kiểm thử đầu cuối cung cấp sẵn dành cho Windows. Bản bàn giao đã được chạy kiểm tra trên Windows.

## 2. Chế độ chạy và vị trí dữ liệu

```powershell
.\build\parking.exe                     # Giờ hệ thống; dữ liệu trong data/
.\build\parking.exe --demo              # Nhập thời gian; dữ liệu trong demo_data/
.\build\parking.exe --demo --data-dir build/my_demo
.\build\parking.exe --help
```

- Khi chưa có dữ liệu, nhập sức chứa từ 1 đến 1.000.000.
- Các thư mục mặc định `data/` và `demo_data/` được tính theo vị trí tệp thực thi trong `build/`, không phụ thuộc thư mục gọi lệnh.
- `--data-dir` là thư mục tùy chọn, đường dẫn tương đối tính theo thư mục làm việc hiện tại. Khi dùng tham số này, bạn chủ động chọn dữ liệu nên hãy chọn một thư mục riêng cho demo.
- Thời gian demo nhập đúng `YYYY-MM-DD HH:MM`, ví dụ `2026-10-26 19:00`.
- Tệp chạy thường và demo ban đầu không có giao dịch. Kiểm thử không sửa các thư mục này.

## 3. Menu

| Lựa chọn | Chức năng |
|---|---|
| 1 | Nhận xe: nhập mã thẻ, biển số, loại xe; giờ vào ở demo |
| 2 | Trả xe: nhập thẻ; đối chiếu biển số; xem phí; xác nhận đã thu tiền |
| 3 | Xe đang gửi và số chỗ còn trống |
| 4 | Tìm toàn bộ lịch sử theo thẻ hoặc biển số chính xác |
| 5 | Xem các lượt đang gửi và đã trả |
| 6 | Thống kê lượt vào, lượt ra, doanh thu theo ngày |
| 7 | Thay đổi sức chứa, không giảm dưới số xe đang gửi |
| 0 | Thoát |

Nhập **`y`** hoặc **`Y`** để xác nhận. Các câu trả lời khác không xác nhận. Khi chưa xác nhận biển số hoặc thu tiền, xe vẫn đang gửi và chưa có doanh thu. Nếu lỗi nhập, thao tác hiện tại bị hủy và menu hiện lại.

Một mã thẻ hợp lệ gồm 1–64 chữ Latin/số/dấu `-`/`_`, không phân biệt chữ hoa thường. Bản mô phỏng chấp nhận thẻ chưa dùng, không cần đăng ký kho thẻ. Biển số được đổi sang chữ hoa, bỏ dấu gạch, dấu chấm và khoảng trắng; kết quả từ 3 đến 20 chữ/số. Ví dụ `29A1-123.45` và `29a1 12345` được coi là cùng xe.

## 4. Biểu phí

Cả hai loại xe dùng cùng mức phí cho **một lượt**:

| Điều kiện | Phí |
|---|---:|
| Vào và ra trước 19:00 cùng ngày | 3.000 đồng |
| Vào từ 19:00, ra cùng ngày | 5.000 đồng |
| Vào trước 19:00, ra từ 19:00 cùng ngày | 7.000 đồng |
| Ngày ra khác ngày vào | Nhập phí nguyên không âm và lý do |

Sáng đến chiều là 3.000 đồng; đúng 19:00 là buổi tối. Vào và ra cùng phút vẫn tính phí theo khung. Phí qua đêm có thể bằng 0 nếu có lý do; lý do tối đa 500 byte UTF-8 trên một dòng. Lượt cùng ngày không được sửa phí thủ công. Biểu phí là yêu cầu của bài tập, không phải thông báo chính thức của trường.

Thời điểm ra được chốt khi tra thẻ. Nếu hủy và quẹt lại thì lấy giờ mới. Doanh thu tính theo **ngày ra**, chỉ gồm các lượt đã xác nhận và lưu thành công.

## 5. Kịch bản demo khi bảo vệ

1. Chạy `--demo`, chọn sức chứa 2 hoặc 3.
2. Nhận C001, biển `29A1-12345`, loại 1, vào `2026-10-26 08:00`.
3. Thử nhận xe khác bằng C001: phải báo thẻ đang dùng.
4. Trả C001 lúc `2026-10-26 17:00`: phí 3.000; xác nhận biển số bằng `y`, chưa thu tiền bằng `n`. Chức năng 3 vẫn có xe.
5. Trả lại C001 lúc `2026-10-26 19:00`: phí 7.000; nhập `y` ở hai bước để hoàn tất.
6. Nhận xe máy điện loại 2 bằng C001 lúc 19:00, trả 21:00: phí 5.000, lịch sử vẫn giữ lượt cũ.
7. Nhận C002 vào ngày 26, trả ngày 27: nhập phí 12.000 và lý do; doanh thu ghi ở ngày 27.
8. Giữ một xe trong bãi, thoát và mở lại để minh họa khôi phục.

## 6. Thiết kế OOP

| Nhóm | Lớp và vai trò |
|---|---|
| Mô hình | `Vehicle`, `ParkingSession`, `ParkingLot`, `DateTime` |
| Tính phí | `FeeRule` trừu tượng; `DayFeeRule`, `NightFeeRule`, `MixedFeeRule` ghi đè |
| Dịch vụ | `ParkingService`: nhận xe, báo giá, xác nhận, tìm kiếm, thống kê |
| Lưu trữ | `ParkingRepository`: tệp văn bản có phiên bản, kiểm tra khi đọc |
| Giao diện | `ConsoleUI`: menu, xác nhận, bắt lỗi |
| Ngoại lệ | `ParkingException` và 5 lớp con theo loại lỗi |

- **Đóng gói:** thuộc tính riêng; thao tác có kiểm tra; trả về tham chiếu `const` khi đọc.
- **Kế thừa:** các quy tắc phí cùng kế thừa `FeeRule`; ngoại lệ kế thừa `ParkingException`.
- **Đa hình:** `FeeCalculator` gọi phương thức ảo thông qua `unique_ptr<FeeRule>`.
- **Quan hệ chứa:** bãi chứa lượt gửi; lượt chứa xe. Xe máy/xe máy điện dùng `enum class` vì hiện cùng cách tính phí.
- **Ngoại lệ:** lớp nghiệp vụ `throw`, giao diện bắt theo tham chiếu lớp cha. Hủy thao tác là nhánh bình thường.

## 7. Dữ liệu và lỗi lưu

`parking_data.txt` có header `PARKING_DATA_V1 <capacity> <nextId>`. Mỗi dòng: mã lượt, thẻ, biển số, loại xe, giờ vào, giờ ra (`-` nếu đang gửi), phí, cờ thủ công và lý do. Chuỗi dùng `std::quoted`.

Một thẻ có nhiều lượt lịch sử nhưng chỉ một lượt đang gửi. Mã lượt không tái sử dụng. Thao tác sửa được làm trên bản dự kiến; chỉ cập nhật bộ nhớ sau khi lưu tệp thành công. Lưu qua `.tmp`, dùng `.bak` trong bước thay thế. Nếu thiếu tệp chính nhưng có `.bak`, chương trình đọc bản sao lưu. Tệp hỏng gây dừng khởi động và không bị ghi đè. Không chỉnh tệp trong khi chương trình đang chạy hoặc mở hai phiên trên cùng dữ liệu.

Nếu cần bãi demo mới, chọn một `--data-dir` mới. Muốn giữ lịch sử cũ thì sao lưu cả thư mục dữ liệu khi chương trình đã thoát.

## 8. Kiểm thử

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\test.ps1
```

Script build và chạy **35 ca C++** bằng `assert`, sau đó build ứng dụng và chạy **4 kịch bản console**. Bao gồm mốc 19:00, cả hai loại xe, trùng thẻ/biển số, bãi đầy, hủy trả, dùng lại thẻ, qua đêm, khởi động lại, tệp hỏng và lỗi ghi. Một ca duyệt toàn bộ 1.037.520 cặp phút hợp lệ trong ngày để kiểm tra ba quy tắc không chồng lấn và không bỏ sót.

Nhật ký: `build/test_results.txt`, `build/cli_results.txt`, `build/cli_demo_output.txt`. Kiểm thử dùng thư mục tạm riêng và dọn phần dữ liệu tự tạo; không đụng `data/` hoặc `demo_data/`. Không biên dịch bộ kiểm thử với `NDEBUG` vì nó sử dụng `assert`.

## 9. Giới hạn

Một máy, một phiên; tra cứu và ghi lại lịch sử tuyến tính theo số lượt. Chưa có camera, thẻ vật lý, phân quyền, vé tháng, xử lý mất thẻ hoặc đồng bộ nhiều cổng. Lưu tệp không thay thế giao dịch/khóa của cơ sở dữ liệu và không bảo đảm chống mọi sự cố phần cứng. Báo cáo chi tiết và sơ đồ nằm ở `../report/parking_report.pdf`.
