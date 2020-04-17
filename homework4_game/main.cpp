#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <list>
#include <vector>
#include <iterator>
#include <unordered_map>
#include <iostream>
#include <fstream>
#include <cmath>
#include <random>

#include <GL/glew.h>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

GLFWwindow* window;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/objloader.hpp>
#include <common/text2D.hpp>


class LoadedModel {
public:
    explicit LoadedModel(const std::string& obj_file, const std::string& texture_file) {
        texture_ = loadBMP_custom(texture_file.data());
        loadOBJ(obj_file.data(), vertices_, uvs_, normals_);
    }

    virtual ~LoadedModel() {
        glDeleteTextures(1, &texture_);
    }

    virtual void Draw(GLuint texture_id, GLuint vertexbuffer, GLuint uvbuffer, GLuint normalbuffer) {
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(glm::vec3), &vertices_[0], GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glBufferData(GL_ARRAY_BUFFER, uvs_.size() * sizeof(glm::vec3), &uvs_[0], GL_STATIC_DRAW);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture_);
        glUniform1i(texture_id, 0);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*) 0);

        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*) 0);

        glDrawArrays(GL_TRIANGLES, 0, vertices_.size());

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }

protected:
    GLuint texture_;
    std::vector<glm::vec3> vertices_;
    std::vector<glm::vec2> uvs_;
    std::vector<glm::vec3> normals_;
};

class Camera {
public:
    explicit Camera(GLfloat horizontal_angle = 0.0f, GLfloat vertical_angle = 0.0f, GLfloat fov = 45.0f)
            : horizontal_angle_(horizontal_angle), vertical_angle_(vertical_angle), fov_(fov) {}

    virtual ~Camera() = default;

    virtual glm::vec3 CameraDirection() {
        return glm::vec3(
                cos(vertical_angle_) * sin(horizontal_angle_),
                sin(vertical_angle_),
                cos(vertical_angle_) * cos(horizontal_angle_)
        );
    }

    virtual glm::vec3 CameraRight() {
        return glm::vec3(
                sin(horizontal_angle_ - 3.14f / 2.0f),
                0,
                cos(horizontal_angle_ - 3.14f / 2.0f)
        );
    }

    virtual glm::vec3 CameraUp() {
        return glm::cross(CameraRight(), CameraDirection());
    }

    virtual GLfloat FOV() {
        return fov_;
    }

protected:
    GLfloat horizontal_angle_;
    GLfloat vertical_angle_;
    GLfloat fov_;
};

class Object : public LoadedModel {
public:
    explicit Object(const glm::vec3& position, const glm::vec3& direction,
                    GLfloat box, GLfloat speed,
                    const std::string& obj_file, const std::string& texture_file)
            : position_(position), direction_(direction), box_(box), speed_(speed),
              LoadedModel(obj_file, texture_file) {}

    ~Object() override = default;

    virtual bool Interact(Object* obj, const glm::vec3& old_position) = 0;

    virtual bool CheckInteraction(Object* obj) {
        return glm::length(position_ - obj->Position()) < (box_ + obj->Box());
    }

    glm::vec3 Position() { return position_; }

    virtual bool CheckSelf() { return true; }

    virtual glm::vec3 GetDirection() { return direction_; };

    float GetSpeed() { return speed_; }

    void Move(const glm::vec3& move) { position_ += move; }

    virtual Object* Act(const std::vector<Object*>& objects) = 0;

    GLfloat Box() { return box_; }

protected:
    glm::vec3 position_;
    glm::vec3 direction_;
    GLfloat box_;
    GLfloat speed_;
};

class Floor : public Object {
public:
    explicit Floor(const glm::vec3& position)
            : Object(position, glm::vec3(0.0f, 1.0f, 0.0f), 1000.0f, 0.0f, "floor.obj", "blood.bmp") {}

    ~Floor() override = default;

    bool Interact(Object* obj, const glm::vec3& old_position) override { return true; }

    bool CheckInteraction(Object* obj) override {
        glm::vec3 diff = obj->Position() - position_;

        return diff.y < obj->Box();
    }

    Object* Act(const std::vector<Object*>& objects) override;
};

class Actor : public Object {
public:
    explicit Actor(const glm::vec3& position, GLfloat box, GLfloat hp, GLfloat speed,
                   const glm::vec3& direction)
            : Object(position, direction, box, speed, "monkey.obj", "zombie.bmp"), hp_(hp) {}

    ~Actor() override = default;

    bool Interact(Object* obj, const glm::vec3& old_position) override;

    void ReceiveDamage(GLfloat damage) { this->hp_ -= damage; };

    Object* Act(const std::vector<Object*>& objects) override { return nullptr; }

protected:
    GLfloat hp_;
};

class Enemy : public Actor {
public:
    explicit Enemy(const glm::vec3& position, const glm::vec3& direction = glm::vec3(0.0f, 0.0f, 0.0f),
                   GLfloat box = 1.0f, GLfloat hp = 1.0f, GLfloat speed = 10.0f, GLfloat interact_damage = 1.0f)
            : Actor(position, box, hp, speed, direction), interact_damage_(interact_damage) {}

    ~Enemy() override = default;

    bool Interact(Object* obj, const glm::vec3& old_position) override;

    Object* Act(const std::vector<Object*>& objects) override;

    GLfloat DealDamage(Object* obj) { return interact_damage_; };
private:
    GLfloat interact_damage_;
};

class FireBall : public Object {
public:
    explicit FireBall(const glm::vec3& position, const glm::vec3& direction, GLfloat box = 0.1f,
                        GLfloat damage = 1.0f, GLfloat speed = 10.0f, GLfloat tl = 10.0f)
            : Object(position, direction, box, speed, "bullet.obj", "fire.bmp"),
              damage_(damage), end_time_(glfwGetTime() + tl) {}

    ~FireBall() override = default;

    bool Interact(Object* obj, const glm::vec3& old_position) override;

    GLfloat DealDamage(Object* obj) { return damage_; };

    bool CheckSelf() override { return glfwGetTime() < end_time_; }

    Object* Act(const std::vector<Object*>& objects) override {
        return nullptr;
    }

protected:
    GLfloat damage_;
    GLfloat end_time_;
};

class Player : public Actor, public Camera {
public:
    explicit Player(const glm::vec3& position = glm::vec3(0.0f, 2.0f, 0.0f),
                    GLfloat box = 1.0f, GLfloat hp = 10.0f, GLfloat speed = 5.0f, GLfloat mouse_speed = 0.005f,
                    GLfloat cooldown = 0.2f, size_t killed = 0)
            : Actor(position, box, hp, speed, glm::vec3(0.0f, 0.0f, 0.0f)),
              mouse_speed_(mouse_speed), cooldown_(cooldown) {}

    ~Player() override = default;

    Object* Act(const std::vector<Object*>& objects) override {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        glfwSetCursorPos(window, 1024 / 2, 768 / 2);
        horizontal_angle_ += mouse_speed_ * GLfloat(1024 / 2 - xpos);
        vertical_angle_ += mouse_speed_ * GLfloat(768 / 2 - ypos);

        if (vertical_angle_ > 3.14f / 2.0f) {
            vertical_angle_ = 3.14f / 2.0f;
        }

        if (vertical_angle_ < -3.14f / 2.0f) {
            vertical_angle_ = -3.14f / 2.0f;
        }

        glm::vec3 direction(
                sin(horizontal_angle_),
                0.0f,
                cos(horizontal_angle_)
        );

        glm::vec3 right = CameraRight();

        glm::vec3 final_direction = glm::vec3(0.0f, 0.0f, 0.0f);

        if (glm::length(final_direction) > 0.0f) {
            final_direction /= glm::length(final_direction);
        }

        direction_ = final_direction;

        glm::vec3 camera_direction = CameraDirection();

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            if (glfwGetTime() > next_projectile_) {
                next_projectile_ = glfwGetTime() + cooldown_;
                FireBall* new_proj = new FireBall(position_ + camera_direction * (box_ + 0.2f),
                                                      camera_direction, 0.1f, 1.0f, 20.0);
                return new_proj;
            }
        }

        return nullptr;
    }

protected:
    GLfloat mouse_speed_;
    GLfloat cooldown_;
    GLfloat next_projectile_ = glfwGetTime();
};

bool Actor::Interact(Object* obj, const glm::vec3& old_position) {
    if (CheckInteraction(obj) && obj->CheckInteraction(this)) {
        auto proj = dynamic_cast<FireBall*>(obj);
        if (proj != nullptr) {
            ReceiveDamage(proj->DealDamage(this));
        }
        if (dynamic_cast<Actor*>(obj) != nullptr) {
            position_ = old_position;
        }
        if (dynamic_cast<Floor*>(obj) != nullptr) {
            position_ = old_position;
        }
        return hp_ > 0.0f;
    } else {
        return true;
    }
}

bool FireBall::Interact(Object* obj, const glm::vec3& old_position) {
    if (CheckInteraction(obj) && obj->CheckInteraction(this)) {
        if (dynamic_cast<Actor*>(obj) != nullptr) {
            return false;
        }
        return dynamic_cast<Floor*>(obj) == nullptr;
    } else {
        return true;
    }
}

bool Enemy::Interact(Object* obj, const glm::vec3& old_position) {
    if (CheckInteraction(obj) && obj->CheckInteraction(this)) {
        auto proj = dynamic_cast<FireBall*>(obj);
        if (proj != nullptr) {
            ReceiveDamage(proj->DealDamage(this));
        }
        if (dynamic_cast<Player*>(obj) != nullptr) {
            auto player = dynamic_cast<Player*>(obj);
            player->ReceiveDamage(this->DealDamage(player));
            return false;
        }
        if (dynamic_cast<Floor*>(obj) != nullptr) {
            position_ = old_position;
        }
        return hp_ > 0.0f;
    } else {
        return true;
    }
}

Object* Enemy::Act(const std::vector<Object*>& objects) {
    Object* target = nullptr;

    for (Object* obj : objects) {
        auto cast_obj = dynamic_cast<Player*>(obj);
        if (cast_obj != nullptr) {
            target = cast_obj;
            break;
        }
    }

    if (target != nullptr) {
        glm::vec3 direction = target->Position() - position_;
        if (glm::length(direction) > 0.0f) {
            direction /= glm::length(direction);
        }

        direction_ = direction;
    }

    return nullptr;
}

Object* Floor::Act(const std::vector<Object*>& objects) {
    Object* target = nullptr;

    for (Object* obj : objects) {
        auto cast_obj = dynamic_cast<Player*>(obj);
        if (cast_obj != nullptr) {
            target = cast_obj;
            break;
        }
    }
    if (target != nullptr) {
        position_ = target->Position() - glm::vec3(0.0f, target->Position().y, 0.0f);
    }
    return nullptr;
}

class EnemyCreator {
public:
    explicit EnemyCreator(GLfloat cooldown = 3.0f, size_t retries = 10, GLfloat r_from = 30.0f,
                          GLfloat r_to = 50.0f, GLfloat hp_from = 1.0f, GLfloat hp_to = 5.0f,
                          GLfloat speed_from = 1.0f, GLfloat speed_to = 5.0f)
            : cooldown_(cooldown), rng_(std::random_device()()), retries_(retries), angle_(-3.14, 3.14),
              r_(r_from, r_to), hp_(hp_from, hp_to), speed_(speed_from, speed_to) {}

    Object* CreateEnemy(const glm::vec3& position, const std::vector<Object*>& objects) {
        if (glfwGetTime() > next_creation_) {
            next_creation_ = glfwGetTime() + cooldown_;

            GLfloat angle_direction = angle_(rng_);
            glm::vec3 orientation(
                    sin(angle_direction),
                    0.0f,
                    cos(angle_direction)
            );

            for (size_t retry = 0; retry < retries_; ++retry) {
                GLfloat angle_position = angle_(rng_);
                glm::vec3 direction(
                        sin(angle_position),
                        0.0f,
                        cos(angle_position)
                );

                GLfloat r = r_(rng_);

                glm::vec3 new_position = position + r * direction;

                Object* new_obj = nullptr;

                new_obj = new Enemy(new_position, orientation, 1.0f, hp_(rng_));

                bool possible = true;

                for (Object* obj : objects) {
                    possible = possible && (!new_obj->CheckInteraction(obj) || !obj->CheckInteraction(new_obj));
                }

                if (possible) {
                    return new_obj;
                }
            }
        }
        return nullptr;
    }

private:
    GLfloat cooldown_;
    size_t retries_;
    GLfloat next_creation_ = glfwGetTime();
    std::mt19937 rng_;
    std::uniform_real_distribution<> angle_;
    std::uniform_real_distribution<> r_;
    std::uniform_real_distribution<> hp_;
    std::uniform_real_distribution<> speed_;
};

int main() {
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        getchar();
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(1024, 768, "Attack on zombie monkeys", nullptr, nullptr);
    if (window == nullptr) {
        fprintf(stderr, "Failed to open GLFW window.\n");
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glewExperimental = true;
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwPollEvents();
    glfwSetCursorPos(window, 1024 / 2, 768 / 2);

    glClearColor(0.00f, 191 / 255.f, 1.0f, 0.0f);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    GLuint simpleProgramID = LoadShaders("ColorVertex.vertexshader", "ColorFragment.fragmentshader");

    GLuint SimpleTextureID = glGetUniformLocation(simpleProgramID, "TextureSampler");
    GLuint SimpleProjectionID = glGetUniformLocation(simpleProgramID, "Projection");
    GLuint SimpleViewID = glGetUniformLocation(simpleProgramID, "View");
    GLuint SimpleModelID = glGetUniformLocation(simpleProgramID, "Model");

    GLuint vertexbuffer;
    glGenBuffers(1, &vertexbuffer);

    GLuint uvbuffer;
    glGenBuffers(1, &uvbuffer);

    GLuint normalbuffer;
    glGenBuffers(1, &normalbuffer);

    Player* player = new Player();

    std::vector<Object*> objects;
    objects.push_back(player);
    objects.push_back(new Floor(player->Position() - glm::vec3(0.0f, player->Position().y, 0.0f)));

    GLfloat prev_time = glfwGetTime();

    EnemyCreator enemy_creator;

    do {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GLfloat current_time = glfwGetTime();

        GLfloat timediff = current_time - prev_time;

        std::vector<glm::vec3> old_positions;

        for (Object* obj : objects) {
            old_positions.push_back(obj->Position());
            obj->Move(obj->GetDirection() * obj->GetSpeed() * timediff);
        }

        prev_time = current_time;

        std::vector<bool> remains(objects.size(), true);

        for (size_t i = 0; i < objects.size(); ++i) {
            remains[i] = objects[i]->CheckSelf();
        }

        for (size_t i = 0; i < objects.size(); ++i) {
            for (size_t j = i + 1; j < objects.size(); ++j) {
                remains[i] = remains[i] && objects[i]->Interact(objects[j], old_positions[i]);
                remains[j] = remains[j] && objects[j]->Interact(objects[i], old_positions[j]);
            }
        }

        if (!remains[0]) {
            break;
        }

        std::vector<Object*> new_objects;

        for (size_t i = 0; i < objects.size(); ++i) {
            if (remains[i]) {
                new_objects.push_back(objects[i]);
            } else {
                delete objects[i];
            }
        }

        std::vector<Object*> act_objects;

        for (size_t i = 0; i < new_objects.size(); ++i) {
            act_objects.push_back(new_objects[i]);
            Object* new_obj = new_objects[i]->Act(new_objects);
            if (new_obj != nullptr) {
                act_objects.push_back(new_obj);
            }
        }

        objects = act_objects;

        Object* new_obj = enemy_creator.CreateEnemy(player->Position(), objects);
        if (new_obj != nullptr) {
            objects.push_back(new_obj);
        }

        glm::mat4 Projection = glm::perspective(glm::radians(player->FOV()), 4.0f / 3.0f, player->Box(), 300.0f);
        glm::mat4 View = glm::lookAt(
                player->Position(),
                player->Position() + player->CameraDirection(),
                player->CameraUp()
        );

        glUseProgram(simpleProgramID);

        glUniformMatrix4fv(SimpleProjectionID, 1, GL_FALSE, &Projection[0][0]);
        glUniformMatrix4fv(SimpleViewID, 1, GL_FALSE, &View[0][0]);

        for (Object* obj : objects) {
            glm::mat4 Scale = glm::scale(glm::mat4(), glm::vec3(obj->Box(), obj->Box(), obj->Box()) / 1.5f);
            glm::mat4 Translate = glm::translate(glm::mat4(), obj->Position());

            glm::vec3 direction = obj->GetDirection();
            glm::vec3 base_direction(1.0f, 0.0f, 0.0f);
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            GLfloat sin_value = glm::dot(glm::cross(base_direction, direction), up);
            GLfloat cos_value = glm::dot(base_direction, direction);

            glm::mat4 Rotate = glm::rotate(glm::mat4(), std::atan2(sin_value, cos_value), up);

            glm::mat4 Model = Translate * Rotate * Scale;
            glUniformMatrix4fv(SimpleModelID, 1, GL_FALSE, &Model[0][0]);

            obj->Draw(SimpleTextureID, vertexbuffer, uvbuffer, normalbuffer);

        }

        glfwSwapBuffers(window);
        glfwPollEvents();

    } while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
             glfwWindowShouldClose(window) == 0);

    for (Object* obj : objects) {
        delete obj;
    }

    glDeleteBuffers(1, &vertexbuffer);
    glDeleteBuffers(1, &uvbuffer);
    glDeleteBuffers(1, &normalbuffer);
    glDeleteProgram(simpleProgramID);
    glDeleteVertexArrays(1, &VertexArrayID);

    glfwTerminate();

    return 0;
}

