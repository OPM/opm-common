#ifndef OPM_OUTPUT_DATA_GUIDERATEVALUE_HPP
#define OPM_OUTPUT_DATA_GUIDERATEVALUE_HPP

#include <array>
#include <bitset>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace Opm { namespace data {

    class GuideRateValue {
    public:
        enum class Item : std::size_t {
            Oil, Gas, Water, ResV,

            // -- Must be last enumerator --
            NumItems,
        };

        void clear()
        {
            this->mask_.reset();
            this->value_.fill(0.0);
        }

        constexpr bool has(const Item p) const
        {
            const auto i = this->index(p);

            return (i < Size) && this->mask_[i];
        }

        bool operator==(const GuideRateValue& vec) const
        {
            return (this->mask_  == vec.mask_)
                && (this->value_ == vec.value_);
        }

        double get(const Item p) const
        {
            if (! this->has(p)) {
                throw std::invalid_argument {
                    "Request for Unset Item Value for " + this->itemName(p)
                };
            }

            return this->value_[ this->index(p) ];
        }

        GuideRateValue& set(const Item p, const double value)
        {
            const auto i = this->index(p);

            if (i >= Size) {
                throw std::invalid_argument {
                    "Cannot Assign Item Value for Unsupported Item '"
                    + this->itemName(p) + '\''
                };
            }

            this->mask_.set(i);
            this->value_[i] = value;

            return *this;
        }

        GuideRateValue& operator+=(const GuideRateValue& rhs)
        {
            for (auto i = 0*Size; i < Size; ++i) {
                if (rhs.mask_[i]) {
                    this->mask_.set(i);
                    this->value_[i] += rhs.value_[i];
                }
            }

            return *this;
        }

        template <class MessageBufferType>
        void write(MessageBufferType& buffer) const
        {
            auto maskrep = this->mask_.to_ullong();
            buffer.write(maskrep);

            for (const auto& x : this->value_) {
                buffer.write(x);
            }
        }

        template <class MessageBufferType>
        void read(MessageBufferType& buffer)
        {
            this->clear();

            {
                auto mask = 0ull;
                buffer.read(mask);

                this->mask_ = std::bitset<Size>(mask);
            }

            for (auto& x : this->value_) {
                buffer.read(x);
            }
        }

    private:
        enum { Size = static_cast<std::size_t>(Item::NumItems) };

        std::bitset<Size>        mask_{};
        std::array<double, Size> value_{};

        constexpr std::size_t index(const Item p) const noexcept
        {
            return static_cast<std::size_t>(p);
        }

        std::string itemName(const Item p) const
        {
            switch (p) {
            case Item::Oil:   return "Oil";
            case Item::Gas:   return "Gas";
            case Item::Water: return "Water";
            case Item::ResV:  return "ResV";

            case Item::NumItems:
                return "Out of bounds (NumItems)";
            }

            return "Unknown (" + std::to_string(this->index(p)) + ')';
        }
    };

}} // namespace Opm::data

#endif // OPM_OUTPUT_DATA_GUIDERATEVALUE_HPP
