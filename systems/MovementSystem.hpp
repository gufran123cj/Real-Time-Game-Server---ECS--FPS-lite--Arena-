#pragma once

#include "../ecs/System.hpp"
#include "../components/InputComponent.hpp"
#include "../components/Velocity.hpp"
#include "../components/Position.hpp"
#include "../components/Transform.hpp"
#include "../physics/Physics.hpp"
#include <cmath>

namespace game::systems {

// Movement System - Converts input to velocity and updates position
// NOT: 2D top-down oyunda yaw rotation'a gerek yok, direkt tuş eşlemesi kullanılıyor
// W/S = Y ekseni, A/D = X ekseni
class MovementSystem
  : public ecs::SystemBase<
        components::InputComponent,
        components::Velocity,
        components::Position,
        components::Transform> {
private:
    const float MOVE_SPEED = 5.0f;        // Units per second
    const float SPRINT_MULTIPLIER = 1.5f;  // Sprint speed multiplier
    const float MAX_SPEED = 10.0f;         // Maximum movement speed

public:
    MovementSystem() = default;

    int getPriority() const override {
        // Movement önce çalışsın
        return 10;
    }

    void process(ecs::World& /*world*/, float deltaTime, ecs::Entity& /*entity*/,
                 components::InputComponent& input,
                 components::Velocity& velocity,
                 components::Position& position,
                 components::Transform& transform) override
    {
        // Top-down 2D: Basit hareket (rotation yok)
        // X: sağ = pozitif, sol = negatif
        // Y: oyun sisteminde yukarı = pozitif, aşağı = negatif
        // Client'ta Y ekseni ters çevriliyor (-player.y), bu yüzden:
        // Oyun sisteminde Y artarsa, ekranda yukarı gider
        
        float moveX = 0.0f;  // Sağ-sol (A/D)
        float moveY = 0.0f;  // Yukarı-aşağı (W/S)
        
        // Direkt tuş eşlemesi (rotation yok)
        // Client'ta Y ekseni ters çevriliyor (-player.y), bu yüzden:
        // Oyun sisteminde Y artarsa, ekranda yukarı gider
        // Oyun sisteminde Y azalırsa, ekranda aşağı gider
        // NOT: Eski input'lar updateRooms()'da temizleniyor (60 tick timeout)
        if (input.isPressed(components::INPUT_FORWARD))  moveY += 1.0f;  // W = yukarı (Y artar, ekranda yukarı)
        if (input.isPressed(components::INPUT_BACKWARD)) moveY -= 1.0f;  // S = aşağı (Y azalır, ekranda aşağı)
        if (input.isPressed(components::INPUT_RIGHT))    moveX += 1.0f;  // D = sağ (X artar)
        if (input.isPressed(components::INPUT_LEFT))     moveX -= 1.0f;  // A = sol (X azalır)

        if (moveX != 0.0f || moveY != 0.0f) {
            // Normalize (çapraz hareket için)
            float len = std::sqrt(moveX * moveX + moveY * moveY);
            moveX /= len;
            moveY /= len;

            // Hız uygula (sprint dahil)
            float speed = MOVE_SPEED * (input.isPressed(components::INPUT_SPRINT) ? SPRINT_MULTIPLIER : 1.0f);
            velocity.value.x = moveX * speed;
            velocity.value.y = moveY * speed;  // Y eksenini kullan
            velocity.value.z = 0.0f;  // Z eksenini kullanma
        } else {
            // Giriş yoksa hızlıca durdur (daha agresif sürtünme)
            velocity.value = velocity.value * 0.3f;  // Daha hızlı sürtünme (0.8'den 0.3'e)
            if (velocity.value.lengthSq() < 0.1f) {  // Threshold'u artırdık (0.01'den 0.1'e)
                velocity.value = physics::Vec3(0.0f, 0.0f, 0.0f);  // Hemen sıfırla
            }
        }

        // 5) Maksimum hız limiti
        float vlen = velocity.value.length();
        if (vlen > MAX_SPEED) {
            velocity.value = velocity.value.normalized() * MAX_SPEED;
        }

        // 6) Konumu güncelle
        position.value = position.value + (velocity.value * deltaTime);

        // 7) Transform senkronu
        transform.position = position.value;
        transform.rotation.y = input.mouseYaw;
    }
};

} // namespace game::systems

