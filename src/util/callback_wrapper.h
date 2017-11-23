/*
 *  util/callback_wrapper.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 23/11/2017.
 *
 */

#ifndef UTIL_CALLBACK_WRAPPER_H
#define UTIL_CALLBACK_WRAPPER_H

#include <memory>

#include <boost/type_traits/function_traits.hpp>

namespace util {

class callback_wrapper_t {
public:
    callback_wrapper_t() : canceled_{std::make_shared<bool>(false)} {}
    ~callback_wrapper_t() { *canceled_ = true; }

    template <typename F>
    auto wrap(F&& cb)
    {
        if(*canceled_) canceled_ = std::make_shared<bool>(false);
        return [canceled = canceled_, cb = std::forward<F>(cb)](auto&&... args) {
            using return_type_t = decltype(cb(args...));
            if(*canceled)
                return return_type_t();

            return cb(std::forward<decltype(args)>(args)...);
        };
    }

    void cancel_callbacks() { *canceled_ = true; }

private:
    std::shared_ptr<bool> canceled_;
};

} // namespace util

#endif // UTIL_CALLBACK_WRAPPER_H
