#pragma once

#include <utility>
#include "esphome/core/component.h"
#include "esphome/core/automation.h"
#include "aqualogic.h"
#include "AquaLogicProto.h"

namespace esphome
{
  namespace aqualogic
  {

    template <typename... Ts>
    class AquaLogicSendAction : public Action<Ts...>
    {
    public:
      explicit AquaLogicSendAction(AquaLogicComponent *aqualogic) : aqualogic_(aqualogic) {}
      TEMPLATABLE_VALUE(uint32_t, key)

      void play(Ts... x) override
      {
        auto val = static_cast<CONTROLLER_KEYS>(this->key_.value(x...));
        this->aqualogic_->send_key(val);
      }

    protected:
      AquaLogicComponent *aqualogic_;
    };

    template <typename... Ts>
    class AquaLogicToggleAction : public Action<Ts...>
    {
     public:
      explicit AquaLogicToggleAction(AquaLogicComponent *aqualogic) : aqualogic_(aqualogic) {}
      TEMPLATABLE_VALUE(uint32_t, key)

      void play(Ts... x) override
      {
        auto val = static_cast<CONTROLLER_KEYS>(this->key_.value(x...));
        this->aqualogic_->send_key_with_retry(val);
      }

    protected:
      AquaLogicComponent *aqualogic_;    };

  }
}