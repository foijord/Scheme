
#include <Scheme.h>

#include <string>
#include <vector>
#include <ranges>
#include <iostream>
#include <algorithm>

scm::env_ptr env = scm::global_env();

bool test(const std::string& exp, const std::string& expected_result)
{
	return [=] {
		std::string result = scm::print(scm::eval(scm::read(exp), env));
		std::cout << exp << " => " << result << " ";
		bool passed = (expected_result == result);
		std::cout << (passed ? "(Pass)" : "(Fail)") << std::endl;
		return passed;
	}();
}

int main(int, char* [])
{
	std::vector<bool> tests{
		test("(quote ())", "()"),
		test("(quote (define a 1))", "(define a 1)"),
		test("(begin (define a 1) (+ 1 2 3))", "6"),
		test("a", "1"),
		test("(quote (testing 1 (2) -3.14e+159))", "(testing 1 (2) -3.14e+159)"),
		test("(+ 2 2)", "4"),
		test("(+ (* 2 100) (* 1 10))", "210"),
		test("(if (> 6 5) (+ 1 1) (+ 2 2))", "2"),
		test("(if (< 6 5) (+ 1 1) (+ 2 2))", "4"),
		test("(define x 3)", "3"),
		test("x", "3"),
		test("(+ x x)", "6"),
		test("((lambda (x) (+ x x)) 5)", "10"),
		test("(define twice (lambda (x) (* 2 x)))", "struct scm::Function"),
		test("(twice 5)", "10"),
		test("(define compose (lambda (f g) (lambda (x) (f (g x)))))", "struct scm::Function"),
		test("((compose list twice) 5)", "(10)"),
		test("(define repeat (lambda (f) (compose f f)))", "struct scm::Function"),
		test("((repeat twice) 5)", "20"),
		test("((repeat (repeat twice)) 5)", "80"),
		test("(define fact (lambda (n) (if (<= n 1) 1 (* n (fact (- n 1))))))", "struct scm::Function"),
		test("(fact 3)", "6"),
		test("(fact 50)", "3.04141e+64"),
		test("(define abs (lambda (n) ((if (> n 0) + -) 0 n)))", "struct scm::Function"),
		test("(list (abs -3) (abs 0) (abs 3))", "(3 0 3)"),
	};

	std::cout << tests.size() << " tests executed." << std::endl;
	std::cout << std::ranges::count(tests, false) << " tests failed." << std::endl;
	std::cout << std::ranges::count(tests, true) << +" tests passed." << std::endl;

	std::cout << "Scheme REPL" << std::endl;

	while (true) {
		try {
			std::cout << "> ";
			std::string input;
			std::getline(std::cin, input);
			std::cout << scm::eval(scm::read(input), env) << std::endl;
		}
		catch (std::exception& e) {
			std::cerr << e.what() << std::endl;
		}
	}

	return EXIT_SUCCESS;
}
