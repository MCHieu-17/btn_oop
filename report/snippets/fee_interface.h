class FeeRule {
public:
    virtual bool matches(const DateTime& entry, const DateTime& exit) const = 0;
    virtual int calculate() const = 0;
    virtual ~FeeRule() = default;
};
