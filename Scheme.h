#pragma once

#include <any>
#include <regex>
#include <vector>
#include <memory>
#include <sstream>
#include <fstream>
#include <numeric>
#include <variant>
#include <numbers>
#include <iostream>
#include <typeinfo>
#include <algorithm>
#include <functional>
#include <unordered_map>

#include <boost/fusion/adapted.hpp>
#include <boost/spirit/home/x3.hpp>

namespace scm {

	typedef std::vector<std::any> List;
	typedef std::shared_ptr<List> lst_ptr;
	typedef std::function<std::any(const List& args)> fun_ptr;

	class Env;
	typedef std::shared_ptr<Env> env_ptr;

	typedef bool Boolean;
	typedef double Number;
	typedef std::string String;

	template <typename Container>
	std::any read(const Container& input);

	struct Symbol : public std::string {
		Symbol() = default;
		explicit Symbol(const String& s) : std::string(s) {}
	};

	struct If {
		std::any test, conseq, alt;
	};

	struct Quote {
		std::any exp;
	};

	struct Define {
		Symbol sym;
		std::any exp;
	};

	struct Lambda {
		std::any parms, body;
	};

	struct Begin {
		lst_ptr exps;
	};

	struct Function {
		std::any parms, body;
		env_ptr env;
	};

	struct Import {
		String code;
	};


	class Env {
	public:
		Env() = default;
		~Env() = default;

		explicit Env(std::unordered_map<std::string, std::any> self)
			: self(std::move(self))
		{}

		Env(const std::any& parm, const List& args, env_ptr outer)
			: outer(std::move(outer))
		{
			if (parm.type() == typeid(lst_ptr)) {
				auto parms = std::any_cast<lst_ptr>(parm);
				for (size_t i = 0; i < parms->size(); i++) {
					auto sym = std::any_cast<Symbol>(parms->at(i));
					self[sym] = args[i];
				}
			}
			else {
				auto sym = std::any_cast<Symbol>(parm);
				self[sym] = args;
			}
		}

		std::any get(Symbol sym)
		{
			if (self.contains(sym)) {
				return self.at(sym);
			}
			if (this->outer) {
				return outer->get(sym);
			}
			throw std::runtime_error("undefined symbol: " + sym);
		}

		std::unordered_map<std::string, std::any> self;
		env_ptr outer{ nullptr };
	};

	template <typename Op>
	Number accumulate(const List& lst)
	{
		std::vector<Number> args(lst.size());
		std::transform(lst.begin(), lst.end(), args.begin(), [](std::any exp) { return std::any_cast<Number>(exp); });
		return std::accumulate(next(args.begin()), args.end(), args.front(), Op());
	}

	env_ptr global_env()
	{
		return std::make_shared<Env>(
			std::unordered_map<std::string, std::any>{
				{ Symbol("pi"), Number(std::numbers::pi) },
				{ Symbol("+"), fun_ptr(accumulate<std::plus<Number>>) },
				{ Symbol("-"), fun_ptr(accumulate<std::minus<Number>>) },
				{ Symbol("/"), fun_ptr(accumulate<std::divides<Number>>) },
				{ Symbol("*"), fun_ptr(accumulate<std::multiplies<Number>>) },
				{ Symbol(">"), fun_ptr([](const List& lst) { return Boolean(std::any_cast<Number>(lst[0]) > std::any_cast<Number>(lst[1])); }) },
				{ Symbol("<"), fun_ptr([](const List& lst) { return Boolean(std::any_cast<Number>(lst[0]) < std::any_cast<Number>(lst[1])); }) },
				{ Symbol("<="), fun_ptr([](const List& lst) { return Boolean(std::any_cast<Number>(lst[0]) <= std::any_cast<Number>(lst[1])); }) },
				{ Symbol(">="), fun_ptr([](const List& lst) { return Boolean(std::any_cast<Number>(lst[0]) >= std::any_cast<Number>(lst[1])); }) },
				{ Symbol("="), fun_ptr([](const List& lst) { return Boolean(std::any_cast<Number>(lst[0]) == std::any_cast<Number>(lst[1])); }) },
				{ Symbol("car"), fun_ptr([](const List& lst) { return std::any_cast<lst_ptr>(lst.front())->front(); }) },
				{ Symbol("cdr"), fun_ptr([](const List& lst) { auto l = std::any_cast<lst_ptr>(lst.front()); return std::make_shared<List>(next(l->begin()), l->end()); }) },
				{ Symbol("list"), fun_ptr([](const List& lst) { return std::make_shared<List>(lst); }) },
				{ Symbol("length"), fun_ptr([](const List& lst) { return static_cast<Number>(std::any_cast<lst_ptr>(lst.front())->size()); }) }
		});
	}

	std::ostream& operator<<(std::ostream& os, const std::any exp)
	{
		if (exp.type() == typeid(lst_ptr)) {
			auto list = std::any_cast<lst_ptr>(exp);
			os << "(";
			if (!list->empty()) {
				for (size_t i = 0; i < list->size(); i++) {
					os << list->at(i);
					if (i != list->size() - 1) {
						os << " ";
					}
				}
			}
			return os << ")";
		}
		if (exp.type() == typeid(Number)) {
			return os << std::any_cast<Number>(exp);
		}
		if (exp.type() == typeid(Symbol)) {
			return os << std::any_cast<Symbol>(exp);
		}
		if (exp.type() == typeid(String)) {
			return os << "\"" << std::any_cast<String>(exp) << "\"";
		}
		if (exp.type() == typeid(Boolean)) {
			return os << (std::any_cast<Boolean>(exp) ? "true" : "false");
		}
		if (exp.type() == typeid(Define)) {
			auto define = std::any_cast<Define>(exp);
			return os << "(define " << define.sym << " " << define.exp << ")";
		}
		if (exp.type() == typeid(Quote)) {
			auto quote = std::any_cast<Quote>(exp);
			return os << "(quote " << quote.exp << ")";
		}

		return os << exp.type().name();
	};

	std::string print(const std::any exp)
	{
		std::stringstream ss;
		ss << exp;
		return ss.str();
	}

	std::any eval(std::any exp, env_ptr env)
	{
		while (true) {
			if (exp.type() == typeid(Number) ||
				exp.type() == typeid(String) ||
				exp.type() == typeid(Boolean)) {
				return exp;
			}
			if (exp.type() == typeid(Symbol)) {
				auto symbol = std::any_cast<Symbol>(exp);
				return env->get(symbol);
			}
			if (exp.type() == typeid(Define)) {
				auto define = std::any_cast<Define>(exp);
				return env->self[define.sym] = eval(define.exp, env);
			}
			if (exp.type() == typeid(Lambda)) {
				auto lambda = std::any_cast<Lambda>(exp);
				return Function{ lambda.parms, lambda.body, env };
			}
			if (exp.type() == typeid(Quote)) {
				auto quote = std::any_cast<Quote>(exp);
				return quote.exp;
			}
			if (exp.type() == typeid(Import)) {
				auto import = std::any_cast<Import>(exp);
				std::any exp = read(import.code);
				return eval(exp, env);
			}
			if (exp.type() == typeid(If)) {
				auto if_ = std::any_cast<If>(exp);
				exp = std::any_cast<Boolean>(eval(if_.test, env)) ? if_.conseq : if_.alt;
			}
			else if (exp.type() == typeid(Begin)) {
				auto begin = std::any_cast<Begin>(exp);
				std::transform(begin.exps->begin(), std::prev(begin.exps->end()), begin.exps->begin(),
					std::bind(eval, std::placeholders::_1, env));
				exp = begin.exps->back();
			}
			else {
				auto list = std::any_cast<lst_ptr>(exp);
				auto call = List(list->size());

				std::transform(list->begin(), list->end(), call.begin(),
					std::bind(eval, std::placeholders::_1, env));

				auto func = call.front();
				auto args = List(std::next(call.begin()), call.end());

				if (func.type() == typeid(Function)) {
					auto function = std::any_cast<Function>(func);
					exp = function.body;
					env = std::make_shared<Env>(function.parms, args, function.env);
				}
				else if (func.type() == typeid(fun_ptr)) {
					auto function = std::any_cast<fun_ptr>(func);
					return function(args);
				}
				else {
					throw std::runtime_error("undefined function: " + print(func));
				}
			}
		}
	}

	typedef std::variant<Number, String, Symbol, Boolean, std::vector<struct value>> value_t;

	struct value : value_t {
		using base_type = value_t;
		using base_type::variant;
	};

	std::any expand(value const& v);

	struct visitor {
		template <typename T>
		std::any operator()(const T& v) const { return v; }

		template <typename T>
		std::any operator()(const std::vector<T>& v) const
		{
			if (v.empty()) {
				return std::make_shared<List>();
			}

			List list(v.size());
			std::transform(v.begin(), v.end(), list.begin(), expand);

			if (list[0].type() == typeid(Symbol)) {
				auto token = std::any_cast<Symbol>(list[0]);

				if (token == "quote") {
					if (list.size() != 2) {
						throw std::invalid_argument("wrong number of arguments to quote");
					}
					return Quote{ .exp = list[1] };
				}
				if (token == "<" || token == ">" || token == "<=" || token == ">=" || token == "==") {
					if (list.size() != 3) {
						throw std::invalid_argument("wrong number of arguments to " + token);
					}
				}
				if (token == "if") {
					if (list.size() != 4) {
						throw std::invalid_argument("wrong number of arguments to if");
					}
					return If{ .test = list[1], .conseq = list[2], .alt = list[3] };
				}
				if (token == "lambda") {
					if (list.size() != 3) {
						throw std::invalid_argument("wrong Number of arguments to lambda");
					}
					return Lambda{ .parms = list[1], .body = list[2] };
				}
				if (token == "begin") {
					if (list.size() < 2) {
						throw std::invalid_argument("wrong Number of arguments to begin");
					}
					return Begin{ .exps = std::make_shared<List>(std::next(list.begin()), list.end()) };
				}
				if (token == "define") {
					if (list.size() < 3 || list.size() > 4) {
						throw std::invalid_argument("wrong number of arguments to define");
					}
					if (list[1].type() != typeid(Symbol)) {
						throw std::invalid_argument("first argument to define must be a Symbol");
					}
					if (list.size() == 3) {
						return Define{ .sym = std::any_cast<Symbol>(list[1]), .exp = list[2] };
					}
					return Define{ .sym = std::any_cast<Symbol>(list[1]), .exp = Lambda{ list[2], list[3] } };
				}
				if (token == "import") {
					if (list.size() != 2) {
						throw std::invalid_argument("wrong number of arguments to import");
					}
					if (list[1].type() != typeid(String)) {
						throw std::invalid_argument("Argument to import must be a String");
					}
					String filename = std::any_cast<String>(list[1]);
					std::ifstream stream(filename, std::ios::in);
					return Import{ std::string{
						std::istreambuf_iterator<char>(stream),
						std::istreambuf_iterator<char>()
					} };
				}
			}
			return std::make_shared<List>(list);
		}
	};

	std::any expand(value const& v)
	{
		visitor expander;
		return std::visit(expander, v);
	}

	namespace parser {
		namespace x3 = boost::spirit::x3;
		using x3::char_;
		using x3::lexeme;
		using x3::double_;

		x3::rule<struct symbol_class, Symbol> symbol_ = "symbol";
		x3::rule<struct number_class, Number> number_ = "number";
		x3::rule<struct string_class, String> string_ = "string";
		x3::rule<struct value_class, value> value_ = "value";
		x3::rule<struct list_class, std::vector<value>> list_ = "list";
		x3::rule<struct multi_string_class, String> multi_string_ = "multi_string";

		struct bool_table : x3::symbols<Boolean> {
			bool_table() {
				add("true", true) ("false", false);
			}
		} const boolean_;

		const auto number__def = double_;
		const auto string__def = lexeme['"' >> *(char_ - '"') >> '"'];
		const auto multi_string__def = lexeme["[[" >> *(char_ - "]]") >> "]]"];
		const auto symbol__def = lexeme[+(char_("A-Za-z") | char_("0-9") | char_('_') | char_('-') | char_("+*/%~&|^!=<>?"))];
		const auto list__def = '(' >> *value_ >> ')';

		const auto value__def
			= number_
			| string_
			| boolean_
			| multi_string_
			| symbol_
			| list_;

		BOOST_SPIRIT_DEFINE(value_, number_, string_, multi_string_, symbol_, list_)

		const auto entry_point = x3::skip(x3::space)[value_];
	}

	template <typename Container>
	std::any read(const Container& input)
	{
		value val;
		visitor expander;

		if (parse(std::begin(input), std::end(input), parser::entry_point, val)) {
			return std::visit(expander, val);
		}
		throw std::runtime_error("Parse failed, remaining input: " + std::string(std::begin(input), std::end(input)));
	}

	template <typename ReturnType, typename InputType>
	std::vector<ReturnType> make_vector(const scm::List& lst)
	{
		std::vector<ReturnType> values(lst.size());
		for (size_t i = 0; i < lst.size(); i++) {
			if (lst[i].type() != typeid(InputType)) {
				throw std::invalid_argument("Invalid argument to make_vector");
			}
			auto value = std::any_cast<InputType>(lst[i]);
			values[i] = static_cast<ReturnType>(value);
		}
		return values;
	};

	template <typename BaseType, typename SubType, typename... Arg, std::size_t... i>
	std::shared_ptr<BaseType> make_shared_object_impl(const List& lst, std::index_sequence<i...>)
	{
		return std::make_shared<SubType>(std::any_cast<Arg>(lst[i])...);
	}

	template <typename BaseType, typename SubType, typename... Arg>
	std::shared_ptr<BaseType> make_shared_object(const List& lst)
	{
		return make_shared_object_impl<BaseType, SubType, Arg...>(lst, std::index_sequence_for<Arg...>{});
	}

	template <typename Type, typename... Arg, std::size_t... i>
	Type make_object_impl(const List& lst, std::index_sequence<i...>)
	{
		return Type(std::any_cast<Arg>(lst[i])...);
	}

	template <typename Type, typename... Arg>
	Type make_object(const List& lst)
	{
		return make_object_impl<Type, Arg...>(lst, std::index_sequence_for<Arg...>{});
	}

}

using scm::operator<<;
