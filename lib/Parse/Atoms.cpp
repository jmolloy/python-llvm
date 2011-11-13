// atom: ('(' [yield_expr|testlist_comp] ')' |
//        '[' [listmaker] ']' |
//        '{' [dictorsetmaker] '}' |
//        '`' testlist1 '`' |
//        NAME | NUMBER | STRING+)
ast::Node *Parser::ParseAtom() {
  switch (T.getKind()) {
  case tok::l_paren:
    return ParseYieldExprOrTestlistComp();
  case tok::l_square:
    return ParseListMaker();
  case tok::l_brace:
    return ParseDictOrSetMaker();
  case tok::backtick:
    return ParseTestlist1();
  case tok::identifier:
    return ParseName();
  case tok::numeric_constant:
    return ParseNumber();
  case tok::string_constant:
    return ParseOneOrMoreStrings();
  default:
    Error(msg::Expected, "an atom");
    return NULL;
  }
}

// .. yield_expr: 'yield' [testlist]
// .. testlist_comp: test ( comp_for | (',' test)* [','] )
ast::Node *Parser::ParseYieldExprOrTestlistComp() {
  assert(T == tok::l_paren);
  L.Lex(T);

  if (T == tok::kw_yield) {
    L.Lex(T);
    ast::TestList *TestList = ParseTestList();
    if (!TestList) return NULL;
    return ast::Yield::Get(TestList);
  }

  ast::Test *Test = ParseTest();
  if (!Test) return NULL;

  if (T == tok::kw_for)
    return ParseCompFor(Test);
  else if (T == tok::comma)
    return ParseTestList(Test);

  Error(msg::Expected2, "for", ",");
  return NULL;
}

// .. comp_for: 'for' exprlist 'in' or_test [comp_iter]
ast::Node *Parser::ParseCompFor(ast::Node *Test) {
  assert(T == tok::kw_for);
  L.Lex(T);

  ast::Node *IndVar = ParseExprList();
  if (!IndVar) return NULL;

  if (T != tok::kw_in) {
    Error(msg::Expected, "in");
    return NULL;
  }

  ast::Node *Container = ParseOrTest();
  if (!Container) return NULL;

  ast::Node *N = ast::Comprehension::Get(Test, IndVar, Container);
  return ParseCompIter(N);
}

// .. comp_if: 'if' old_test [comp_iter]
ast::Node *Parser::ParseCompIf(ast::Node *N) {
  ast::Comprehension *C = llvm::cast<ast::Comprehension>(N);

  assert(T == tok::kw_if);
  L.Lex(T);

  ast::Test *T = ParseOldTest();
  if (!T) return NULL;

  if (C->GetPredicate())
    Warning(msg::MultipleComprehensionPredicates);
  else
    C->SetPredicate(T);

  return N;    
}

// .. comp_iter: comp_for | comp_if
ast::Node *Parser::ParseCompIter(ast::Node *N) {
  switch (T.getKind()) {
  case tok::kw_for:
    return ParseCompFor(N);
  case tok::kw_if:
    return ParseCompIf(N);
  default:
    return N;
  }
}
