#include "../ramen/rgl_math.h"
#include <chrono>
#include <vector>

class MatrixStack {
    public:
        MatrixStack() {
            matrixStack.push_back(Mat4f::Identity()); // Initialisiert Stack mit Identitätsmatrix.
        }
        void Push() {
            matrixStack.push_back(Last());
        }
        void Pop() {
            if (!matrixStack.empty()) {
                matrixStack.pop_back();
            }
        }
        Mat4f& Last() {
            return matrixStack.back(); // currentMatrix.
        }
        void Rotate(const Vec3f& axis, float angle) {
            Last() = Last() * ::Rotate(axis, angle);
        }
        void Translate(const Vec3f& translation) {
            Last() = Last() * ::Translate(translation);
        }
        void Scale(const Vec3f& scale) {
            Last() = Last() * ::Scale(scale);
        }
    private:
        std::vector<Mat4f> matrixStack;
};
