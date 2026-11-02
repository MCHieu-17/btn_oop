#include "services/ParkingService.h"
#include "exceptions/ParkingException.h"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace {
std::filesystem::path testRoot;
int passed = 0;
int failed = 0;

DateTime at(const std::string& value) { return DateTime::parse(value); }
Vehicle bike(const std::string& plate = "29A1-123.45") { return Vehicle(plate, VehicleType::Motorbike); }
LostTicketInfo lostInfo(const std::string& studentId = "SV001",
                        const std::string& studentName = "Nguyen Van A",
                        const std::string& ownerName = "Tran Van B") {
    return LostTicketInfo(studentId, studentName, ownerName, "001234567890", "K66",
                          "0912345678", "Xe cua bo");
}
std::filesystem::path file(const std::string& name) { return testRoot / name / "parking_data.txt"; }
void write(const std::filesystem::path& path, const std::string& content) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << content;
}
std::string read(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::binary);
    std::ostringstream out;
    out << in.rdbuf();
    return out.str();
}
template<class Exception, class Function>
void expectThrow(Function action) {
    bool caught = false;
    try { action(); } catch (const Exception&) { caught = true; }
    assert(caught);
}
void run(const std::string& name, const std::function<void()>& action) {
    try {
        action();
        ++passed;
        std::cout << "PASS " << name << '\n';
    } catch (const std::exception& error) {
        ++failed;
        std::cout << "FAIL " << name << ": " << error.what() << '\n';
    }
}
}

int main() {
    const auto nonce = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    testRoot = std::filesystem::temp_directory_path() / ("parking_oop_tests_" + std::to_string(nonce));
    std::filesystem::create_directories(testRoot);
    FeeCalculator calculator;
    run("T01_valid_calendar_and_leap_year", [] {
        assert(at("2024-02-29 23:59").toString() == "2024-02-29 23:59");
        assert(at("2000-02-29 00:00") < at("2000-03-01 00:00"));
    });
    run("T02_reject_invalid_datetime", [] {
        for (const std::string value : {"2026-02-29 10:00", "1900-02-29 10:00", "2026-04-31 10:00",
             "2026-10-26 24:00", "2026-10-26 19:60", "2026-1-26 19:00", "0000-01-01 00:00"}) {
            expectThrow<ValidationException>([&] { at(value); });
        }
    });
    run("T03_normalize_card_and_plate", [] {
        assert(bike(" 29a1 - 123.45 ").getPlate() == "29A112345");
        assert(normalizeCardId(" c_001 ") == "C_001");
        expectThrow<ValidationException>([] { bike(""); });
        expectThrow<ValidationException>([] { normalizeCardId("C 001"); });
        expectThrow<ValidationException>([] { Vehicle("29A12345", static_cast<VehicleType>(3)); });
    });
    run("T04_morning_to_afternoon_3000", [&] {
        assert(calculator.calculate(at("2026-10-26 08:00"), at("2026-10-26 17:00")) == 3000);
    });
    run("T05_before_1900_3000", [&] {
        assert(calculator.calculate(at("2026-10-26 18:00"), at("2026-10-26 18:59")) == 3000);
    });
    run("T06_exit_exactly_1900_7000", [&] {
        assert(calculator.calculate(at("2026-10-26 18:00"), at("2026-10-26 19:00")) == 7000);
    });
    run("T07_entry_exactly_1900_5000", [&] {
        assert(calculator.calculate(at("2026-10-26 19:00"), at("2026-10-26 21:00")) == 5000);
    });
    run("T08_morning_to_night_7000", [&] {
        assert(calculator.calculate(at("2026-10-26 08:00"), at("2026-10-26 21:00")) == 7000);
    });
    run("T09_same_minute_is_one_visit", [&] {
        assert(calculator.calculate(at("2026-10-26 08:00"), at("2026-10-26 08:00")) == 3000);
        assert(calculator.calculate(at("2026-10-26 19:00"), at("2026-10-26 19:00")) == 5000);
    });
    run("T10_no_auto_fee_for_overnight_or_reversed_time", [&] {
        expectThrow<ValidationException>([&] { calculator.calculate(at("2026-10-26 20:00"), at("2026-10-27 08:00")); });
        expectThrow<ValidationException>([&] { calculator.calculate(at("2026-10-26 20:00"), at("2026-10-26 08:00")); });
    });
    run("T11_fee_rules_cover_all_1037520_minute_pairs", [] {
        DayFeeRule day;
        NightFeeRule night;
        MixedFeeRule mixed;
        long long pairs = 0;
        for (int entry = 0; entry < 1440; ++entry) {
            const DateTime in(2026, 10, 26, entry / 60, entry % 60);
            for (int exit = entry; exit < 1440; ++exit) {
                const DateTime out(2026, 10, 26, exit / 60, exit % 60);
                const int count = day.matches(in, out) + night.matches(in, out) + mixed.matches(in, out);
                assert(count == 1);
                if (exit < 1140) assert(day.matches(in, out));
                else if (entry >= 1140) assert(night.matches(in, out));
                else assert(mixed.matches(in, out));
                ++pairs;
            }
        }
        assert(pairs == 1037520);
    });
    run("T12_two_vehicle_types_share_fee", [] {
        ParkingService service(ParkingRepository(file("types")), 2);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        service.checkIn("C2", Vehicle("29M-99999", VehicleType::ElectricMotorbike), at("2026-10-26 08:00"));
        assert(service.prepareCheckout("C1", at("2026-10-26 17:00")).fee ==
               service.prepareCheckout("C2", at("2026-10-26 17:00")).fee);
    });
    run("T13_checkin_persists_and_generates_id", [] {
        const auto path = file("entry");
        ParkingService service(ParkingRepository(path), 2);
        assert(service.checkIn("c1", bike(), at("2026-10-26 08:00")) == 1);
        const auto restored = ParkingRepository(path).load();
        assert(restored.activeCount() == 1 && restored.getNextId() == 2);
        assert(restored.findActiveByCard("C1").getVehicle().getPlate() == "29A112345");
    });
    run("T14_duplicate_card_rejected", [] {
        ParkingService service(ParkingRepository(file("duplicate_card")), 3);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        expectThrow<ConflictException>([&] { service.checkIn("c1", bike("29A199999"), at("2026-10-26 09:00")); });
        assert(service.getLot().activeCount() == 1 && service.getLot().getNextId() == 2);
    });
    run("T15_duplicate_normalized_plate_rejected", [] {
        ParkingService service(ParkingRepository(file("duplicate_plate")), 3);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        expectThrow<ConflictException>([&] { service.checkIn("C2", bike("29a1 12345"), at("2026-10-26 09:00")); });
    });
    run("T16_full_lot_rejected", [] {
        ParkingService service(ParkingRepository(file("full")), 1);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        expectThrow<CapacityException>([&] { service.checkIn("C2", bike("29A199999"), at("2026-10-26 09:00")); });
    });
    run("T17_missing_card_and_invalid_exit", [] {
        ParkingService service(ParkingRepository(file("missing")), 2);
        expectThrow<NotFoundException>([&] { service.prepareCheckout("C9", at("2026-10-26 17:00")); });
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        expectThrow<ValidationException>([&] { service.prepareCheckout("C1", at("2026-10-26 07:59")); });
    });
    run("T18_quote_does_not_close_or_collect_money", [] {
        const auto path = file("quote");
        ParkingService service(ParkingRepository(path), 2);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        const auto before = read(path);
        const auto quote = service.prepareCheckout("C1", at("2026-10-26 19:00"));
        assert(quote.fee == 7000);
        assert(service.getLot().activeCount() == 1 && service.statistics("2026-10-26").revenue == 0);
        assert(read(path) == before);
    });
    run("T19_confirm_checkout_and_revenue", [] {
        ParkingService service(ParkingRepository(file("checkout")), 2);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        auto quote = service.prepareCheckout("C1", at("2026-10-26 19:00"));
        quote.fee = 1; // Xác nhận phải tính lại phí, không tin số tiền trong báo giá.
        service.confirmCheckout(quote);
        assert(service.getLot().activeCount() == 0);
        const auto stat = service.statistics("2026-10-26");
        assert(stat.entries == 1 && stat.exits == 1 && stat.revenue == 7000);
        expectThrow<NotFoundException>([&] { service.confirmCheckout(quote); });
    });
    run("T20_reuse_card_and_reject_stale_quote", [] {
        ParkingService service(ParkingRepository(file("reuse")), 2);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        const auto old = service.prepareCheckout("C1", at("2026-10-26 17:00"));
        service.confirmCheckout(old);
        assert(service.checkIn("C1", bike(), at("2026-10-26 17:00")) == 2);
        expectThrow<ConflictException>([&] { service.confirmCheckout(old); });
        assert(service.searchByCard("c1").size() == 2 && service.getLot().activeCount() == 1);
    });
    run("T21_overnight_requires_valid_amount_and_reason", [] {
        ParkingService service(ParkingRepository(file("overnight_validation")), 1);
        service.checkIn("C1", bike(), at("2026-10-26 18:00"));
        const auto quote = service.prepareCheckout("C1", at("2026-10-27 08:00"));
        assert(quote.manualRequired && !quote.fee);
        expectThrow<ValidationException>([&] { service.confirmCheckout(quote); });
        expectThrow<ValidationException>([&] { service.confirmCheckout(quote, -1, "Thu qua dem"); });
        expectThrow<ValidationException>([&] { service.confirmCheckout(quote, 10000, "   "); });
        assert(service.getLot().activeCount() == 1);
    });
    run("T22_overnight_revenue_belongs_to_exit_date", [] {
        ParkingService service(ParkingRepository(file("overnight_revenue")), 1);
        service.checkIn("C1", bike(), at("2026-10-31 18:00"));
        service.confirmCheckout(service.prepareCheckout("C1", at("2026-11-01 08:00")), 12000, "Thu theo xử lý qua đêm");
        assert(service.statistics("2026-10-31").revenue == 0);
        const auto result = service.statistics("2026-11-01");
        assert(result.entries == 0 && result.exits == 1 && result.revenue == 12000);
    });
    run("T23_capacity_change_validated_and_persisted", [] {
        const auto path = file("capacity");
        ParkingService service(ParkingRepository(path), 2);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        service.checkIn("C2", bike("29A199999"), at("2026-10-26 08:00"));
        expectThrow<CapacityException>([&] { service.changeCapacity(1); });
        service.changeCapacity(3);
        assert(ParkingRepository(path).load().getCapacity() == 3);
        expectThrow<CapacityException>([&] { service.changeCapacity(0); });
    });
    run("T24_restart_restores_active_history_and_next_id", [] {
        const auto path = file("restart");
        {
            ParkingService service(ParkingRepository(path), 3);
            service.checkIn("C1", bike(), at("2026-10-26 08:00"));
            service.confirmCheckout(service.prepareCheckout("C1", at("2026-10-26 17:00")));
            service.checkIn("C2", bike("29A199999"), at("2026-10-26 18:00"));
        }
        ParkingService restored(ParkingRepository(path), 999);
        assert(restored.getLot().getCapacity() == 3 && restored.getLot().activeCount() == 1);
        assert(restored.getLot().getSessions().size() == 2 && restored.getLot().getNextId() == 3);
        assert(restored.statistics("2026-10-26").revenue == 3000);
    });
    run("T25_corrupt_header_never_overwritten", [] {
        const auto path = file("bad_header");
        write(path, "BROKEN DATA\n");
        const auto before = read(path);
        expectThrow<StorageException>([&] { ParkingService service(ParkingRepository(path), 10); });
        assert(read(path) == before);
    });
    run("T26_wrong_stored_fee_rejected", [] {
        const auto path = file("bad_fee");
        write(path, "PARKING_DATA 10 2\n1 \"C1\" \"29A12345\" 1 \"2026-10-26 08:00\" \"2026-10-26 17:00\" 1 0 \"\" 0 \"\" \"\" \"\" \"\" \"\" \"\" \"\"\n");
        expectThrow<StorageException>([&] { ParkingRepository(path).load(); });
    });
    run("T27_duplicate_active_cards_in_file_rejected", [] {
        const auto path = file("bad_cards");
        write(path, "PARKING_DATA 10 3\n1 \"C1\" \"29A12345\" 1 \"2026-10-26 08:00\" \"-\" 0 0 \"\" 0 \"\" \"\" \"\" \"\" \"\" \"\" \"\"\n2 \"c1\" \"29A99999\" 1 \"2026-10-26 08:00\" \"-\" 0 0 \"\" 0 \"\" \"\" \"\" \"\" \"\" \"\" \"\"\n");
        expectThrow<StorageException>([&] { ParkingRepository(path).load(); });
    });
    run("T28_invalid_stored_date_and_next_id_rejected", [] {
        const auto path = file("bad_date");
        write(path, "PARKING_DATA 10 2\n1 \"C1\" \"29A12345\" 1 \"2026-02-30 08:00\" \"-\" 0 0 \"\" 0 \"\" \"\" \"\" \"\" \"\" \"\" \"\"\n");
        expectThrow<StorageException>([&] { ParkingRepository(path).load(); });
        write(path, "PARKING_DATA 10 1\n1 \"C1\" \"29A12345\" 1 \"2026-10-26 08:00\" \"-\" 0 0 \"\" 0 \"\" \"\" \"\" \"\" \"\" \"\" \"\"\n");
        expectThrow<StorageException>([&] { ParkingRepository(path).load(); });
    });
    run("T29_failed_save_preserves_entry_state_and_disk", [] {
        const auto path = file("fail_entry");
        ParkingService service(ParkingRepository(path), 2);
        const auto before = read(path);
        std::filesystem::create_directory(std::filesystem::path(path).concat(".tmp"));
        expectThrow<StorageException>([&] { service.checkIn("C1", bike(), at("2026-10-26 08:00")); });
        assert(service.getLot().activeCount() == 0 && service.getLot().getNextId() == 1);
        assert(read(path) == before);
    });
    run("T30_failed_save_preserves_checkout_and_revenue", [] {
        const auto path = file("fail_exit");
        ParkingService service(ParkingRepository(path), 2);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        const auto before = read(path);
        const auto quote = service.prepareCheckout("C1", at("2026-10-26 17:00"));
        std::filesystem::create_directory(std::filesystem::path(path).concat(".tmp"));
        expectThrow<StorageException>([&] { service.confirmCheckout(quote); });
        assert(service.getLot().activeCount() == 1 && service.statistics("2026-10-26").revenue == 0);
        assert(read(path) == before);
    });
    run("T31_restore_backup_after_interrupted_replace", [] {
        const auto path = file("backup");
        {
            ParkingService service(ParkingRepository(path), 2);
            service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        }
        const auto backup = std::filesystem::path(path).concat(".bak");
        std::filesystem::rename(path, backup);
        ParkingService service(ParkingRepository(path), 999);
        assert(service.getLot().activeCount() == 1 && service.getLot().getCapacity() == 2);
        service.changeCapacity(3);
        assert(std::filesystem::exists(path) && ParkingRepository(path).load().getCapacity() == 3);
    });
    run("T32_search_history_by_normalized_plate", [] {
        ParkingService service(ParkingRepository(file("search")), 2);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        service.confirmCheckout(service.prepareCheckout("C1", at("2026-10-26 17:00")));
        service.checkIn("C2", bike(), at("2026-10-26 18:00"));
        assert(service.searchByPlate("29a1-123.45").size() == 2);
        assert(service.searchByCard("C1").size() == 1);
        assert(service.searchByCard("UNKNOWN").empty());
    });
    run("T33_same_day_manual_override_rejected", [] {
        ParkingService service(ParkingRepository(file("override")), 1);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        const auto quote = service.prepareCheckout("C1", at("2026-10-26 17:00"));
        expectThrow<ValidationException>([&] { service.confirmCheckout(quote, 0, "Mien phi"); });
        assert(service.getLot().activeCount() == 1);
    });
    run("T34_quoted_unicode_reason_roundtrip_and_zero_fee", [] {
        const auto path = file("unicode_reason");
        ParkingService service(ParkingRepository(path), 1);
        service.checkIn("C1", bike(), at("2026-10-26 18:00"));
        const std::string reason = "Miễn phí theo xác nhận \"trực đêm\", hồ sơ A\\B";
        service.confirmCheckout(service.prepareCheckout("C1", at("2026-10-27 08:00")), 0, reason);
        const auto restored = ParkingRepository(path).load();
        assert(restored.getSessions()[0].getReason() == reason);
        assert(restored.getSessions()[0].hasManualFee() && restored.getSessions()[0].getPaidFee() == 0);
    });
    run("T35_reject_invalid_statistics_date_and_overflow", [] {
        ParkingService service(ParkingRepository(file("overflow")), 1);
        expectThrow<ValidationException>([&] { service.statistics("2026-02-30"); });
        service.checkIn("C1", bike(), at("2026-10-25 08:00"));
        service.confirmCheckout(service.prepareCheckout("C1", at("2026-10-26 08:00")),
                                std::numeric_limits<long long>::max(), "Kiem thu gioi han so nguyen");
        service.checkIn("C1", bike(), at("2026-10-26 09:00"));
        service.confirmCheckout(service.prepareCheckout("C1", at("2026-10-26 17:00")));
        expectThrow<ValidationException>([&] { service.statistics("2026-10-26"); });
    });
    run("T36_lost_ticket_same_day_manual_fee", [] {
        const auto path = file("lost_same_day");
        ParkingService service(ParkingRepository(path), 2);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        const auto quote = service.prepareLostTicketCheckout("29a1-123.45", at("2026-10-26 17:00"));
        assert(quote.manualRequired && quote.fee == 3000);
        service.confirmLostTicketCheckout(quote, 50000, lostInfo());
        assert(service.getLot().activeCount() == 0);
        const auto stat = service.statistics("2026-10-26");
        assert(stat.entries == 1 && stat.exits == 1 && stat.revenue == 50000);
        const auto restored = ParkingRepository(path).load();
        assert(restored.getSessions().size() == 1);
        const auto& session = restored.getSessions()[0];
        assert(session.hasLostTicket() && session.hasManualFee() && session.getPaidFee() == 50000);
        assert(session.getLostTicket()->getStudentId() == "SV001");
        assert(session.getLostTicket()->getOwnerName() == "Tran Van B");
    });
    run("T37_lost_ticket_requires_student_and_owner_info", [] {
        ParkingService service(ParkingRepository(file("lost_validation")), 1);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        const auto quote = service.prepareLostTicketCheckout("29A1-12345", at("2026-10-26 17:00"));
        expectThrow<ValidationException>([&] {
            service.confirmLostTicketCheckout(quote, 50000, LostTicketInfo("", "A", "B"));
        });
        expectThrow<ValidationException>([&] {
            service.confirmLostTicketCheckout(quote, 50000, LostTicketInfo("SV001", "", "B"));
        });
        expectThrow<ValidationException>([&] {
            service.confirmLostTicketCheckout(quote, 50000, LostTicketInfo("SV001", "A", ""));
        });
        expectThrow<ValidationException>([&] {
            service.confirmLostTicketCheckout(quote, -1, lostInfo());
        });
        assert(service.getLot().activeCount() == 1);
    });
    run("T38_lost_ticket_unknown_plate", [] {
        ParkingService service(ParkingRepository(file("lost_unknown")), 1);
        expectThrow<NotFoundException>([&] {
            service.prepareLostTicketCheckout("29A1-99999", at("2026-10-26 17:00"));
        });
    });
    run("T39_lost_ticket_roundtrip_unicode_and_optional_fields", [] {
        const auto path = file("lost_roundtrip");
        ParkingService service(ParkingRepository(path), 1);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        const LostTicketInfo info("SV001", "Nguyễn Văn A", "Trần Văn B", "", "K66", "",
                                  "Xe của bố");
        service.confirmLostTicketCheckout(
            service.prepareLostTicketCheckout("29A1-12345", at("2026-10-26 17:00")), 0, info);
        const auto restored = ParkingRepository(path).load();
        const auto& session = restored.getSessions()[0];
        assert(session.hasLostTicket());
        const auto& lost = *session.getLostTicket();
        assert(lost.getStudentName() == "Nguyễn Văn A");
        assert(lost.getOwnerName() == "Trần Văn B");
        assert(lost.getOwnerId().empty() && lost.getPhone().empty());
        assert(lost.getNote() == "Xe của bố");
        assert(session.getPaidFee() == 0 && session.hasManualFee());
    });
    run("T40_lost_ticket_does_not_block_card_reuse", [] {
        ParkingService service(ParkingRepository(file("lost_reuse")), 2);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        service.confirmLostTicketCheckout(
            service.prepareLostTicketCheckout("29A1-12345", at("2026-10-26 17:00")), 50000,
            lostInfo());
        assert(service.checkIn("C1", bike("29A1-99999"), at("2026-10-26 18:00")) == 2);
    });
    run("T41_lost_ticket_stale_quote_rejected", [] {
        ParkingService service(ParkingRepository(file("lost_stale")), 2);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        const auto quote = service.prepareLostTicketCheckout("29A1-12345", at("2026-10-26 17:00"));
        service.confirmLostTicketCheckout(quote, 50000, lostInfo());
        expectThrow<NotFoundException>([&] {
            service.confirmLostTicketCheckout(quote, 50000, lostInfo());
        });
    });
    run("T42_failed_lost_ticket_save_preserves_state", [] {
        const auto path = file("lost_fail");
        ParkingService service(ParkingRepository(path), 1);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        const auto before = read(path);
        const auto quote = service.prepareLostTicketCheckout("29A1-12345", at("2026-10-26 17:00"));
        std::filesystem::create_directory(std::filesystem::path(path).concat(".tmp"));
        expectThrow<StorageException>([&] {
            service.confirmLostTicketCheckout(quote, 50000, lostInfo());
        });
        assert(service.getLot().activeCount() == 1 && service.statistics("2026-10-26").revenue == 0);
        assert(read(path) == before);
    });
    run("T43_invalid_lost_record_rejected", [] {
        const auto path = file("bad_lost");
        write(path, "PARKING_DATA 10 2\n1 \"C1\" \"29A12345\" 1 \"2026-10-26 08:00\" \"-\" 0 0 \"\" 1 \"SV001\" \"A\" \"B\" \"\" \"\" \"\" \"\"\n");
        expectThrow<StorageException>([&] { ParkingRepository(path).load(); });
    });
    run("T44_non_lost_record_with_lost_fields_rejected", [] {
        const auto path = file("bad_non_lost");
        write(path, "PARKING_DATA 10 2\n1 \"C1\" \"29A12345\" 1 \"2026-10-26 08:00\" \"2026-10-26 17:00\" 3000 0 \"\" 0 \"SV001\" \"\" \"\" \"\" \"\" \"\" \"\"\n");
        expectThrow<StorageException>([&] { ParkingRepository(path).load(); });
    });
    run("T45_lost_ticket_quoted_fields_roundtrip", [] {
        const auto path = file("lost_quoted");
        ParkingService service(ParkingRepository(path), 1);
        service.checkIn("C1", bike(), at("2026-10-26 08:00"));
        const LostTicketInfo info("SV\"001", "Nguyen \"A\"", "Tran \\ B", "001", "K66",
                                  "0912", "Ghi chu \"dac biet\" \\");
        service.confirmLostTicketCheckout(
            service.prepareLostTicketCheckout("29A1-12345", at("2026-10-26 17:00")), 50000, info);
        const auto restored = ParkingRepository(path).load();
        const auto& lost = *restored.getSessions()[0].getLostTicket();
        assert(lost.getStudentId() == "SV\"001");
        assert(lost.getStudentName() == "Nguyen \"A\"");
        assert(lost.getOwnerName() == "Tran \\ B");
        assert(lost.getNote() == "Ghi chu \"dac biet\" \\");
    });
    std::cout << "RESULT " << passed << " passed, " << failed << " failed\n";
    std::error_code cleanupError;
    std::filesystem::remove_all(testRoot, cleanupError);
    return failed == 0 ? 0 : 1;
}
