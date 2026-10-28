# Báo cáo dự án - Đại học Kinh tế Quốc dân

Bản báo cáo thiết kế lại theo bố cục học thuật một cột. Mục tiêu khoảng **40 trang cho toàn bộ PDF** (38–42 trang), gồm bìa, thông tin nhóm, tóm tắt, mục lục, danh mục hình/bảng, bảy chương và phụ lục. Nội dung không dùng citation hoặc mục tài liệu tham khảo.

**Bản PDF bàn giao hiện có đúng 40 trang.** Tất cả các trang đã được render và rà soát; kiểm tra tự động không phát hiện lỗi ký tự thay thế, tham chiếu thiếu hoặc chữ tràn lề. Font thân bài và đoạn mã đã được nhúng vào PDF.

## Tệp cần dùng

- `parking_report.pdf`: bản đọc và in.
- `parking_report.tex`: tệp LaTeX chính.
- `metadata.tex`: tên trường, thông tin lớp, giảng viên, nhóm và ba sinh viên.
- `preamble.tex`: kiểu chữ, lề trang, tiêu đề, chú thích và cách trình bày đoạn mã.
- `chapters/`: nội dung từng phần.
- `figures/`: sơ đồ TikZ, nhật ký kiểm thử và trích đầu ra console thực tế.
- `snippets/`: các đoạn mã được lấy từ chương trình để phân tích trong báo cáo.

## Điền thông tin trước khi nộp

Tên trường đã đặt là **ĐẠI HỌC KINH TẾ QUỐC DÂN**. Trong `metadata.tex`, điền:

1. `MemberOne`, `StudentOne`: họ tên và mã sinh viên thứ nhất.
2. `MemberTwo`, `StudentTwo`: họ tên và mã sinh viên thứ hai.
3. `MemberThree`, `StudentThree`: họ tên và mã sinh viên thứ ba.
4. Khoa/viện, lớp học phần, giảng viên, tên nhóm và năm nộp nếu cần điều chỉnh.

Bìa và trang thông tin dùng cùng các biến nên chỉ cần điền một lần. Bảng phân công là dự kiến; nhóm cập nhật theo đóng góp thực tế. Phần xác nhận đóng góp được chừa trống.

## Biên dịch

Từ thư mục dự án:

```powershell
powershell -ExecutionPolicy Bypass -File .\report\build.ps1
```

Từ thư mục `report`:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

Script ưu tiên Tectonic portable hoặc XeLaTeX. Nếu chưa có công cụ, script tải bản Tectonic Windows x64 đã cố định phiên bản vào `.tools/`; lần đầu cần Internet để lấy các gói LaTeX. PDF cuối được đặt ở `parking_report.pdf`, tệp trung gian và nhật ký ở `build/`.

Nếu dùng TeX Live hoặc MiKTeX có XeLaTeX:

```powershell
powershell -ExecutionPolicy Bypass -File .\build.ps1 -Engine XeLaTeX
```

Không dùng pdfLaTeX vì nguồn sử dụng `fontspec`. Trên máy đã tạo bản bàn giao, font là Times New Roman và Consolas; trên máy thiếu font, nguồn có lựa chọn dự phòng. Thân bài 13 pt, giãn dòng 1,25; mã nguồn 10,5 pt. Khổ A4, lề trái 3 cm, phải 2 cm, trên/dưới 2,5 cm.

## Nội dung và dữ liệu kiểm tra

Các chương lần lượt phân tích bối cảnh, yêu cầu, mô hình lớp, OOP, cài đặt, kiểm thử và kết luận. Phần giải thích gắn với các tình huống dùng lại thẻ, hủy trả, mốc 19:00 và lỗi lưu; không có số liệu khảo sát hoặc đóng góp cá nhân được tự đặt.

Kết quả đối chiếu: **35 ca C++ đạt, bốn kịch bản console đạt**. Nhật ký ở `figures/test_results.txt`, `figures/cli_results.txt`; đầu ra minh họa ở `figures/demo_summary.txt`. Nguồn chương trình nằm tại `../source_code/`; các đoạn mã trong `snippets/` là bản trích dùng cho báo cáo, không phải bộ mã để build ứng dụng.

Sau khi sửa code, cần cập nhật các đoạn trích và nội dung liên quan trước khi biên dịch lại. Sau khi điền tên thật hoặc thay nội dung, kiểm tra lại số trang, bảng, hình và ngắt trang. Mốc 40 trang là tổng chiều dài tài liệu, không phải số trang nội dung chính.
