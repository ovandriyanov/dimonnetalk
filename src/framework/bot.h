/*
 *  framework/bot.h
 *  dimonnetalk
 *
 *  Created by Oleg Andriyanov on 27/11/2017.
 *
 */

#ifndef FRAMEWORK_BOT_H
#define FRAMEWORK_BOT_H

namespace framework {

class bot_t
{
public:
    virtual ~bot_t() = default;
    virtual void reload() = 0;
    virtual void stop() = 0;
};

} // namespace framework

#endif // FRAMEWORK_BOT_H
