CheckoutQuote ParkingService::prepareCheckout(const std::string& cardId, const DateTime& exit) const {
    const auto& session = lot_.findActiveByCard(cardId);
    if (exit < session.getEntry()) throw ValidationException("Giờ ra không được trước giờ vào.");
    const bool overnight = !session.getEntry().sameDate(exit);
    std::optional<long long> fee;
    if (!overnight) fee = calculator_.calculate(session.getEntry(), exit);
    return {session.getId(), session.getCardId(), session.getVehicle().getPlate(),
            session.getEntry(), exit, fee, overnight};
}
