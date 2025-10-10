# Creating Domains

This guide shows you how to create custom domains in V Engine. Domains are the primary way to add modular systems to your application, whether you're creating game logic, rendering systems, or utility services.

## Understanding Domains

Domains are modular systems that can act as entities, components, and systems simultaneously. They are the building blocks of your application and can be combined in flexible ways.

### Domain Types

1. **Standard Domains**: Game logic, AI, physics, etc.
2. **Render Domains**: GPU rendering tasks and resources
3. **Singleton Domains**: Systems that should only have one instance

## Basic Domain Creation

### 1. Header File

```cpp
// my_domain.h
#pragma once

#include <engine/domain.h>

namespace game {
    class MyDomain : public v::Domain<MyDomain> {
    public:
        MyDomain(v::Engine& engine);
        ~MyDomain() override = default;

        void init_standard_components() override;

        // Your public methods here
        void update_game_logic();
        void handle_input();

    private:
        // Your member variables here
        float game_timer_;
        bool is_active_;
    };
}
```

### 2. Implementation File

```cpp
// my_domain.cpp
#include "my_domain.h"
#include <engine/engine.h>

namespace game {
    MyDomain::MyDomain(v::Engine& engine)
        : v::Domain(engine, "MyDomain"),  // Domain name for debugging
          game_timer_(0.0f),
          is_active_(false)
    {
        // Constructor logic here
        std::cout << "MyDomain created!" << std::endl;
    }

    void MyDomain::init_standard_components() {
        // Initialize components and setup system behavior
        is_active_ = true;

        // Add components to this domain's entity if needed
        engine().emplace<GameStateComponent>(entity(),
            GameStateComponent{
                .score = 0,
                .level = 1,
                .is_paused = false
            }
        );
    }

    void MyDomain::update_game_logic() {
        if (!is_active_) return;

        // Get delta time from engine
        float dt = static_cast<float>(engine().delta_time());
        game_timer_ += dt;

        // Your game logic here
        if (game_timer_ > 1.0f) {
            std::cout << "Game logic update!" << std::endl;
            game_timer_ = 0.0f;
        }
    }

    void MyDomain::handle_input() {
        // Access input context
        auto* input_ctx = engine().get_ctx<v::WindowContext>();
        if (!input_ctx) return;

        // Handle input logic
        if (input_ctx->is_key_just_pressed(v::Key::SPACE)) {
            std::cout << "Space key pressed!" << std::endl;
        }
    }
}
```

### 3. Registration and Usage

```cpp
// In your application setup
#include "my_domain.h"

void setup_game(v::Engine& engine) {
    // Create and register the domain
    auto* my_domain = engine.add_domain<game::MyDomain>();

    // The domain is now available for querying
    auto* domain = engine.get_domain<game::MyDomain>();
    domain->update_game_logic();
}
```

## Singleton Domains

For systems that should only have one instance, use `SDomain`:

```cpp
// game_manager.h
#pragma once

#include <engine/domain.h>

namespace game {
    class GameManager : public v::SDomain<GameManager> {
    public:
        GameManager(v::Engine& engine);

        void init_standard_components() override;

        // Game state management
        void start_new_game();
        void pause_game();
        void resume_game();
        void game_over();

        // Getters
        bool is_game_running() const { return game_state_ == GameState::RUNNING; }
        int get_score() const { return score_; }

    private:
        enum class GameState {
            MENU,
            RUNNING,
            PAUSED,
            GAME_OVER
        };

        GameState game_state_;
        int score_;
        int level_;
    };
}
```

## Domain Communication

### Querying Other Domains

```cpp
class PlayerDomain : public v::Domain<PlayerDomain> {
public:
    void update_player(v::Engine& engine) {
        // Query other domains
        auto* terrain = engine.try_get_domain<TerrainDomain>();
        auto* ui = engine.try_get_domain<UIDomain>();

        // Use other domains if they exist
        if (terrain) {
            // Check collision with terrain
            auto player_pos = get_player_position();
            auto height = terrain->get_height_at(player_pos.x, player_pos.z);
            update_player_height(height);
        }

        if (ui) {
            // Update UI with player stats
            ui->update_health_display(get_health());
            ui->update_score_display(get_score());
        }
    }
};
```

### Component-Based Communication

```cpp
// Define a shared component
struct PlayerEventComponent {
    enum class EventType {
        DAMAGED,
        HEALED,
        LEVEL_UP,
        ITEM_COLLECTED
    };

    EventType type;
    int value;
    std::string description;
};

// In one domain
class CombatDomain : public v::Domain<CombatDomain> {
public:
    void damage_player(v::Engine& engine, int damage) {
        // Create event component
        PlayerEventComponent event{
            .type = PlayerEventComponent::EventType::DAMAGED,
            .value = damage,
            .description = "Player took " + std::to_string(damage) + " damage"
        };

        // Send to all domains that listen for player events
        for (auto [entity, event_listener] :
             engine.view<PlayerEventListenerComponent>().each()) {
            event_listener.on_event(event);
        }
    }
};
```

## Task Integration

### Registering Update Tasks

```cpp
class MyDomain : public v::Domain<MyDomain> {
public:
    MyDomain(v::Engine& engine) : v::Domain(engine, "MyDomain") {
        // Register update tasks
        engine.on_tick.connect(
            {"input"},           // After input is processed
            {"physics"},         // Before physics runs
            "my_domain_update", // Task name
            [this]() { update(); }
        );

        engine.on_tick.connect(
            {"my_domain_update"}, // After our update
            {"render"},           // Before rendering
            "my_domain_prepare_render",
            [this]() { prepare_render_data(); }
        );
    }

private:
    void update() {
        // Main update logic
    }

    void prepare_render_data() {
        // Prepare data for rendering
    }
};
```

### Dependent Tasks

```cpp
void setup_tasks(v::Engine& engine) {
    // Define task dependencies

    // Input processing (no dependencies)
    engine.on_tick.connect({}, {}, "input_process", []() {
        // Process input
    });

    // Player movement (depends on input)
    engine.on_tick.connect({"input_process"}, {}, "player_movement", []() {
        // Update player based on input
    });

    // Camera update (depends on player)
    engine.on_tick.connect({"player_movement"}, {}, "camera_update", []() {
        // Update camera to follow player
    });

    // Physics (depends on movement)
    engine.on_tick.connect({"player_movement", "camera_update"}, {}, "physics", []() {
        // Run physics simulation
    });

    // Rendering (depends on everything else)
    engine.on_tick.connect({"physics", "camera_update"}, {}, "render", []() {
        // Render frame
    });
}
```

## Resource Management

### Managing Domain Resources

```cpp
class AudioDomain : public v::Domain<AudioDomain> {
public:
    AudioDomain(v::Engine& engine) : v::Domain(engine, "Audio") {
        // Load audio resources
        load_audio_files();
    }

    ~AudioDomain() {
        // Resources automatically cleaned up
    }

    void play_sound(const std::string& sound_name) {
        auto it = sounds_.find(sound_name);
        if (it != sounds_.end()) {
            it->second.play();
        }
    }

private:
    void load_audio_files() {
        // Load sounds into memory
        sounds_["jump"] = Sound("assets/jump.wav");
        sounds_["shoot"] = Sound("assets/shoot.wav");
        sounds_["explosion"] = Sound("assets/explosion.wav");
    }

    std::unordered_map<std::string, Sound> sounds_;
};
```

### Lazy Resource Loading

```cpp
class TextureDomain : public v::Domain<TextureDomain> {
public:
    TextureDomain(v::Engine& engine) : v::Domain(engine, "Texture") {
        // Don't load all textures immediately
    }

    Texture* get_texture(const std::string& texture_name) {
        auto it = textures_.find(texture_name);

        if (it == textures_.end()) {
            // Load texture on demand
            Texture texture = load_texture_from_file("assets/textures/" + texture_name + ".png");
            auto result = textures_.emplace(texture_name, std::move(texture));
            it = result.first;
        }

        return &it->second;
    }

private:
    Texture load_texture_from_file(const std::string& path) {
        // Load texture from disk
        Texture texture;
        texture.load_from_file(path);
        return texture;
    }

    std::unordered_map<std::string, Texture> textures_;
};
```

## Domain Lifecycle

### Construction and Initialization

```cpp
class MyDomain : public v::Domain<MyDomain> {
public:
    MyDomain(v::Engine& engine) : v::Domain(engine, "MyDomain") {
        // Constructor called immediately when domain is created
        std::cout << "MyDomain constructor called" << std::endl;

        // Don't access other domains here - they might not exist yet
        // Use init_standard_components() for domain initialization
    }

    void init_standard_components() override {
        // Called after all domains are created
        // Safe to access other domains here
        std::cout << "MyDomain initialization called" << std::endl;

        // Setup components and behavior
        setup_components();
        register_update_tasks();
    }

private:
    void setup_components() {
        // Add components to this domain's entity
        engine().emplace<MyComponent>(entity(), /* component data */);
    }

    void register_update_tasks() {
        // Register update tasks with engine
        engine().on_tick.connect({}, {}, "my_domain_update",
            [this]() { this->update(); });
    }
};
```

### Cleanup and Destruction

```cpp
class MyDomain : public v::Domain<MyDomain> {
public:
    ~MyDomain() {
        // Destructor called when domain is destroyed
        // Resources are automatically cleaned up

        std::cout << "MyDomain destructor called" << std::endl;

        // Save state if needed
        save_game_state();

        // Cleanup custom resources (smart pointers handle most cleanup)
        cleanup_custom_resources();
    }

private:
    void save_game_state() {
        // Save game state to file
    }

    void cleanup_custom_resources() {
        // Cleanup any non-RAII resources
    }
};
```

## Advanced Patterns

### Domain Factory Pattern

```cpp
class DomainFactory {
public:
    template<typename DomainType, typename... Args>
    static DomainType* create_domain(v::Engine& engine, Args&&... args) {
        auto* domain = engine.add_domain<DomainType>(std::forward<Args>(args)...);

        // Setup common domain features
        setup_common_features(domain, engine);

        return domain;
    }

private:
    template<typename DomainType>
    static void setup_common_features(DomainType* domain, v::Engine& engine) {
        // Add common components or setup common tasks
        domain->set_update_rate(60.0f);  // 60 FPS update rate
    }
};

// Usage
auto* player_domain = DomainFactory::create_domain<PlayerDomain>(engine, player_config);
auto* terrain_domain = DomainFactory::create_domain<TerrainDomain>(engine, terrain_config);
```

### Domain Configuration

```cpp
struct DomainConfig {
    float update_rate = 60.0f;
    bool enable_debug = false;
    std::string resource_path = "assets/";
};

class ConfigurableDomain : public v::Domain<ConfigurableDomain> {
public:
    ConfigurableDomain(v::Engine& engine, const DomainConfig& config)
        : v::Domain(engine, "ConfigurableDomain"), config_(config) {

        if (config_.enable_debug) {
            setup_debug_features();
        }
    }

    void set_update_rate(float rate) {
        config_.update_rate = rate;
    }

private:
    DomainConfig config_;

    void setup_debug_features() {
        // Setup debug logging, UI, etc.
    }
};
```

## Error Handling

### Domain Initialization Errors

```cpp
class MyDomain : public v::Domain<MyDomain> {
public:
    MyDomain(v::Engine& engine) : v::Domain(engine, "MyDomain") {
        // Validate resources in constructor
        if (!validate_resources()) {
            throw std::runtime_error("Failed to initialize MyDomain: Missing required resources");
        }
    }

    void init_standard_components() override {
        try {
            // Initialize components that might fail
            initialize_components();
        } catch (const std::exception& e) {
            // Handle initialization failure
            std::cerr << "Failed to initialize MyDomain: " << e.what() << std::endl;

            // Set domain to inactive state
            set_active(false);
        }
    }

private:
    bool validate_resources() {
        // Check if required files exist
        return std::filesystem::exists("assets/required_file.txt");
    }

    void initialize_components() {
        // Component initialization that might throw
    }
};
```

### Safe Domain Access

```cpp
void process_game_logic(v::Engine& engine) {
    // Safe access to optional domains
    auto* player = engine.try_get_domain<PlayerDomain>();
    if (!player) {
        std::cerr << "Warning: PlayerDomain not found" << std::endl;
        return;
    }

    auto* terrain = engine.try_get_domain<TerrainDomain>();
    auto* ui = engine.try_get_domain<UIDomain>();

    // Process with available domains
    player->update(terrain, ui);
}
```

## Testing Domains

### Unit Testing Domains

```cpp
// Test fixture for domain testing
class DomainTest : public ::testing::Test {
protected:
    void SetUp() override {
        engine_ = std::make_unique<v::Engine>();
    }

    void TearDown() override {
        engine_.reset();
    }

    std::unique_ptr<v::Engine> engine_;
};

TEST_F(DomainTest, PlayerDomainCreation) {
    // Test domain creation
    auto* player_domain = engine_->add_domain<PlayerDomain>();

    EXPECT_NE(player_domain, nullptr);
    EXPECT_EQ(engine_->get_domain<PlayerDomain>(), player_domain);
}

TEST_F(DomainTest, PlayerDomainFunctionality) {
    auto* player_domain = engine_->add_domain<PlayerDomain>();

    // Test domain functionality
    player_domain->set_position({1.0f, 2.0f, 3.0f});
    auto position = player_domain->get_position();

    EXPECT_FLOAT_EQ(position.x, 1.0f);
    EXPECT_FLOAT_EQ(position.y, 2.0f);
    EXPECT_FLOAT_EQ(position.z, 3.0f);
}
```

## Best Practices

1. **Keep Domains Focused**: Each domain should have a single responsibility
2. **Use Interfaces**: Define interfaces for domain communication to reduce coupling
3. **Handle Missing Dependencies**: Always use `try_get_domain()` for optional dependencies
4. **Resource Management**: Use RAII and smart pointers for automatic cleanup
5. **Error Handling**: Validate resources and handle initialization failures gracefully
6. **Testing**: Create unit tests for domain functionality
7. **Documentation**: Document domain interfaces and usage patterns

## Common Pitfalls to Avoid

1. **Circular Dependencies**: Don't create domains that depend on each other in a cycle
2. **Constructor Dependencies**: Don't access other domains in constructors
3. **Memory Leaks**: Use smart pointers to manage heap allocations
4. **Performance**: Avoid querying for domains in hot paths (cache references)
5. **Thread Safety**: Be careful when sharing data between threads