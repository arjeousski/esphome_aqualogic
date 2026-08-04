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
      TEMPLATABLE_VALUE(uint16_t, type)

      void play(Ts... x) override
      {
        auto val = static_cast<CONTROLLER_KEYS>(this->key_.value(x...));
        auto type_val = this->type_.value(x...);
        this->aqualogic_->send_key(val, type_val);
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
      TEMPLATABLE_VALUE(uint16_t, type)

      void play(Ts... x) override
      {
        auto val = static_cast<CONTROLLER_KEYS>(this->key_.value(x...));
        auto type_val = this->type_.value(x...);
        this->aqualogic_->send_key_with_retry(val, type_val);
      }

    protected:
      AquaLogicComponent *aqualogic_;    };

    template <typename... Ts>
    class AquaLogicPressAction : public Action<Ts...>
    {
    public:
      explicit AquaLogicPressAction(AquaLogicComponent *aqualogic) : aqualogic_(aqualogic) {}
      TEMPLATABLE_VALUE(uint32_t, key)
      TEMPLATABLE_VALUE(uint16_t, type)

      void play(Ts... x) override
      {
        auto val = static_cast<CONTROLLER_KEYS>(this->key_.value(x...));
        auto type_val = this->type_.value(x...);
        this->aqualogic_->press_key(val, type_val);
      }

    protected:
      AquaLogicComponent *aqualogic_;
    };

    template <typename... Ts>
    class AquaLogicReleaseAction : public Action<Ts...>
    {
    public:
      explicit AquaLogicReleaseAction(AquaLogicComponent *aqualogic) : aqualogic_(aqualogic) {}

      void play(Ts... x) override
      {
        this->aqualogic_->release_key();
      }

    protected:
      AquaLogicComponent *aqualogic_;
    };

  }
}