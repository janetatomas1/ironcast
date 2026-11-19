
#include <ironcast/ironcast.hpp>

namespace ironcast {
    namespace testing {
        struct Test {
            std::string name;
            std::function<std::unique_ptr<Widget>()> scene;
            std::function<void(Widget&)> body;
        };

        void add_test(std::unique_ptr<Test> test);
        const std::vector<std::unique_ptr<Test>>& tests();
        int run(int argc, char** argv);
    }
}
