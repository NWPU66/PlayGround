#include <cstdlib>
#include <queue>
#include <vector>

#include <gtest/gtest.h>

int Factorial(int n)
{
    int result = 1;
    for (; n > 1; n--)
    {
        result *= n;
    }
    return result;
}

template <typename E> class Queue {
public:
    void Enqueue(const E& element) { q.push(element); }
    E    Dequeue()
    {
        if (!q.empty())
        {
            E front = q.front();
            q.pop();
            return front;
        }
    }
    size_t size() const { return q.size(); }

private:
    std::queue<E> q;
};

TEST(VectorTest, SizeEquality)
{
    std::vector<float> a = {1, 2, 3, 4, 5};
    std::vector<float> b = {1, 2, 3, 4, 5};
    ASSERT_EQ(a.size(), b.size()) << "Vector sizes are not equal!";
    EXPECT_EQ(a.size(), b.size()) << "Vector sizes are not equal!";  // non fatal error

    for (int i = 0; i < a.size(); ++i)
    {
        EXPECT_EQ(a[i], b[i]) << "Vectors x and y differ at index " << i;
    }
}

// Tests factorial of 0.
TEST(FactorialTest, HandlesZeroInput)
{
    EXPECT_EQ(Factorial(0), 1);
}

// Tests factorial of positive numbers.
TEST(FactorialTest, HandlesPositiveInput)
{
    EXPECT_EQ(Factorial(1), 1);
    EXPECT_EQ(Factorial(2), 2);
    EXPECT_EQ(Factorial(3), 6);
    EXPECT_EQ(Factorial(8), 40320);
}

class QueueTest : public testing::Test {
protected:
    Queue<int> q0_;
    Queue<int> q1_;
    Queue<int> q2_;

    void SetUp() override
    {
        q1_.Enqueue(1);
        q2_.Enqueue(2);
        q2_.Enqueue(3);
    }

    void TearDown() override
    {
        // clean up
    }
};

TEST_F(QueueTest, IsEmptyInitially)
{
    EXPECT_EQ(q0_.size(), 0);
}

TEST_F(QueueTest, DequeueWorks)
{
    int n = q0_.Dequeue();

    n = q1_.Dequeue();
    EXPECT_EQ(n, 1);
    EXPECT_EQ(q1_.size(), 0);

    n = q2_.Dequeue();
    EXPECT_EQ(n, 2);
    EXPECT_EQ(q2_.size(), 1);
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
