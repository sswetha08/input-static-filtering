#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"

#include "llvm/Support/CommandLine.h"

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include "clang/AST/Comment.h"
#include "clang/AST/CommentVisitor.h"
#include "clang/AST/RecursiveASTVisitor.h"

#include <iostream>
#include <vector>
#include <map>

using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace clang::comments;
using namespace llvm;

std::vector<std::string> inputVars;
std::map<int, std::string> keyPoints;


// Create a map of var names to represent flowing value relatioship (input-dependence list)
std::map<std::string, std::string> valFlow;

// AST matcher to match getc I/O calls
StatementMatcher getcMatcher = callExpr(
  callee(functionDecl(hasName("getc"))),
  hasAncestor(
    binaryOperator(hasOperatorName("=")).bind("binaryOp")
  )
).bind("getcCall");

// AST matcher to match scanf I/O calls
StatementMatcher scanfMatcher =
  callExpr(callee(functionDecl(hasName("scanf")))).bind("scanfCall");

// Define a matcher for label statements
StatementMatcher labelMatcher = labelStmt().bind("label");

// Matcher for Assign statements containing var references
StatementMatcher assignMatcher = binaryOperator(
    hasOperatorName("="),
    hasLHS(declRefExpr().bind("lhs")),
    hasRHS(expr(hasDescendant(declRefExpr())).bind("rhs"))
).bind("assign");

// Matcher for comparison with EOF character
StatementMatcher eofCheck = binaryOperator(
      anyOf(hasOperatorName("=="), hasOperatorName("!=")),
      hasRHS(parenExpr(has(unaryOperator(hasOperatorName("-"),
      hasUnaryOperand(integerLiteral(equals(1))))
      )))).bind("comparison");

class EOFHandler : public MatchFinder::MatchCallback {
public:
  virtual void run(const MatchFinder::MatchResult &Result) {
    const auto *binaryOp = Result.Nodes.getNodeAs<BinaryOperator>("comparison"); // Extract Binary operator node containing comparison
    const auto* lhs = dyn_cast<Expr>(binaryOp->getLHS());
    const auto* node = lhs->IgnoreImplicit()->IgnoreParens(); // ignore implicit type casts
    if (const auto *Recvnode = dyn_cast<DeclRefExpr>(node)) {
        std::string name = Recvnode->getNameInfo().getAsString();
        auto it = valFlow.find(name);  // check if var referenced is in the input dependence list
        if (it != valFlow.end()) {
        std::cout <<"\n*Variable "<<name<<" does not directly affect execution, file length may be critical!\n";
        }
      } 
  }
};

// Callback for traversing all var references
class VarRefHandler : public MatchFinder::MatchCallback {
public:
  virtual void run(const MatchFinder::MatchResult &Result) {
    const DeclRefExpr *DRE = Result.Nodes.getNodeAs<DeclRefExpr>("declrefexpr");
    const SourceManager &SM = *Result.SourceManager;
    std::string varName = DRE->getNameInfo().getAsString();
    auto it = valFlow.find(varName);  
    if (it != valFlow.end()) {
        SourceLocation Loc = DRE->getLocation();
        int refLineNo = SM.getPresumedLineNumber(Loc);
        auto itr = keyPoints.find(refLineNo);
        if(itr != keyPoints.end()) // check if variable is referenced at the defined keypoints
        {
          std::cout << "\nInput variable: " << it->second <<" may determine the program's execution path!"<<std::endl;
          std::cout << "Reason: variable " << it->first <<" affects "<<itr->second<<" at line: "<<SM.getPresumedLineNumber(Loc)<<" and its value is influenced by input."<<std::endl;
        }
    }
  }
};

// Define the callback function to handle matched statements
class AssignHandler : public MatchFinder::MatchCallback {
public:
  virtual void run(const MatchFinder::MatchResult &Result) {
    // Get the matched AST nodes
    const auto *LhsDeclRef = Result.Nodes.getNodeAs<DeclRefExpr>("lhs");
    const auto *RhsExpr = Result.Nodes.getNodeAs<Expr>("rhs");
    if (LhsDeclRef && RhsExpr) {
      // llvm::errs() << "Found assignment statement with LHS: " << LhsDeclRef->getNameInfo().getAsString() << "\n";
      // llvm::errs() << "RHS DeclRefExprs: \n";
      std::string dest = LhsDeclRef->getNameInfo().getAsString();
      const auto *RhsSubExpr = RhsExpr->IgnoreImplicit()->IgnoreParens();
      // if RHS contains only one var ref
      if (const auto *RhsDeclRef = dyn_cast<DeclRefExpr>(RhsSubExpr)) { 
        // llvm::errs() << "\t" << RhsDeclRef->getNameInfo().getAsString() << "\n";
        std::string name = RhsDeclRef->getNameInfo().getAsString();
        auto it = valFlow.find(name);  
        if (it != valFlow.end()) {
          std::string parent = it->second;
          valFlow.insert({ dest, parent }); 
        }
      } 
      // RHS is an expr with a binary op
      else if (const auto *binaryRHS = dyn_cast<BinaryOperator>(RhsSubExpr))
      {
          const auto* lhsRHS = dyn_cast<Expr>(binaryRHS->getLHS());
          const auto* lhsRHSI = lhsRHS->IgnoreImplicit()->IgnoreParens();
          const auto* rhsRHS = dyn_cast<Expr>(binaryRHS->getRHS());
          const auto* rhsRHSI = rhsRHS->IgnoreImplicit()->IgnoreParens();

          const auto* l = dyn_cast<DeclRefExpr>(lhsRHSI);
          const auto* r = dyn_cast<DeclRefExpr>(rhsRHSI);

          if (l || r) {
            if (l)
            {
              // llvm::errs() << "RHS LHS: " << l->getNameInfo().getName().getAsString() << "\n";
              std::string lname = l->getNameInfo().getName().getAsString();
              auto it = valFlow.find(lname);  
              // check if lhs of binary operator is present in input-dependence list
              if (it != valFlow.end()) { 
                std::string parent = it->second;
                valFlow.insert({ dest, parent }); // add the destination variable of Assign to the list
              }
            }             
            if (r)
            {
              // llvm::errs() << "RHS RHS: " << r->getNameInfo().getName().getAsString() << "\n";
              std::string rname = r->getNameInfo().getName().getAsString();
              auto it = valFlow.find(rname);  
              // check if rhs of binary operator is present in input-dependence list
              if (it != valFlow.end()) { 
                std::string parent = it->second;
                valFlow.insert({ dest, parent }); // add the destination variable of Assign to the list
              }
            }
          }        
      }
    }
  }
};

// Callback to add key points
class LabelPrinter : public MatchFinder::MatchCallback {
public:
  virtual void run(const MatchFinder::MatchResult &Result) {
    // Get the matched statement
    const LabelStmt *Label = Result.Nodes.getNodeAs<LabelStmt>("label");
    if (Label) {
      StringRef LabelName = Label->getName();
        if (LabelName.contains("key_point")) {
          int lineNo = Result.Context->getSourceManager().getSpellingLineNumber(Label->getBeginLoc());
          // llvm::outs() << LabelName
          //             << " at line " << Result.Context->getSourceManager().getSpellingLineNumber(Label->getBeginLoc())
          //             << "\n";
          keyPoints.insert({lineNo, LabelName.str()});
      }
    }
  }
};

class CommentFinder : public clang::ASTConsumer {
public:
  CommentFinder(clang::ASTContext  &ASTContext) {
  }

  void HandleTranslationUnit(clang::ASTContext &context) final {
    auto comments = context.Comments.getCommentsInFile(
      context.getSourceManager().getMainFileID());
    llvm::outs() << context.Comments.empty() << "\n";
    if(!context.Comments.empty())
    {
      for (auto it = comments->begin(); it != comments->end(); it++) {
        clang::RawComment* comment = it->second;
        std::string source = comment->getFormattedText(context.getSourceManager(),
            context.getDiagnostics());
        llvm::outs() << "source:" << source << "\n";
      }
    }
  }
};

class MyFrontendAction : public clang::ASTFrontendAction {
public:
  std::unique_ptr<clang::ASTConsumer> CreateASTConsumer(
    clang::CompilerInstance& CI, clang::StringRef) final {
    return std::unique_ptr<clang::ASTConsumer>(
        new CommentFinder(CI.getASTContext()));
  }
};

// Extract input variables from the matched scanf AST
class ScanfExtractor : public MatchFinder::MatchCallback {
public :
  virtual void run(const MatchFinder::MatchResult &Result) {
    if (const CallExpr* ScanfCall = Result.Nodes.getNodeAs<CallExpr>("scanfCall")) {
      llvm::outs() << "Found a scanf call with variables:";
      for (unsigned i = 1; i < ScanfCall->getNumArgs(); i++) {
        const Expr* ArgExpr = ScanfCall->getArg(i);
        if (const UnaryOperator* UnOp = dyn_cast<UnaryOperator>(ArgExpr)) {
          if (UnOp->getOpcode() == UO_AddrOf) {
            const Expr* AddrExpr = UnOp->getSubExpr();
            if (const DeclRefExpr* VarRef = dyn_cast<DeclRefExpr>(AddrExpr)) {
              const VarDecl* Var = dyn_cast<VarDecl>(VarRef->getDecl());
              if (Var) {
                llvm::outs() << " " << Var->getName();
                inputVars.push_back(Var->getNameAsString());
                valFlow.insert({Var->getNameAsString(), Var->getNameAsString()});
              }
            }
          }
        } else if (const DeclRefExpr* VarRef = dyn_cast<DeclRefExpr>(ArgExpr)) {
          const VarDecl* Var = dyn_cast<VarDecl>(VarRef->getDecl());
          if (Var) {
            llvm::outs() << " " << Var->getNameAsString();
            inputVars.push_back(Var->getNameAsString());
            valFlow.insert({Var->getNameAsString(), Var->getNameAsString()});
          }
        }
      }
      llvm::outs() << "\n";
    }
  }
};

// Extract input variables from the matched getc AST
class GetcExtractor : public MatchFinder::MatchCallback {
public :
    virtual void run(const MatchFinder::MatchResult &Result) {
    const auto *BinaryOp = Result.Nodes.getNodeAs<BinaryOperator>("binaryOp");
    if (BinaryOp) {
      const auto *Var = dyn_cast<DeclRefExpr>(BinaryOp->getLHS()->IgnoreImpCasts());
      if (Var) {
        const auto *VarDecl = Var->getDecl();
        llvm::outs() << "Found getc() call assigned to variable: " << VarDecl->getNameAsString() << "\n";
        inputVars.push_back(VarDecl->getNameAsString());
        valFlow.insert({VarDecl->getNameAsString(), VarDecl->getNameAsString()});
      }
    }
    }
};

// Apply a custom category to all command-line options so that they are the
// only ones displayed.
static llvm::cl::OptionCategory MyToolCategory("my-tool options");

// CommonOptionsParser declares HelpMessage with a description of the common
// command-line options related to the compilation database and input files.
// It's nice to have this help message in all tools.
static cl::extrahelp CommonHelp(CommonOptionsParser::HelpMessage);

// A help message for this specific tool can be added afterwards.
static cl::extrahelp MoreHelp("\nMore help text...\n");

int main(int argc, const char **argv) {
  auto ExpectedParser = CommonOptionsParser::create(argc, argv, MyToolCategory);
  if (!ExpectedParser) {
    // Fail gracefully for unsupported options.
    llvm::errs() << ExpectedParser.takeError();
    return 1;
  }
  CommonOptionsParser& OptionsParser = ExpectedParser.get();
  ClangTool Tool(OptionsParser.getCompilations(),
                 OptionsParser.getSourcePathList());

  GetcExtractor getcHandler;
  ScanfExtractor scanfHandler;
  MatchFinder Finder;
  LabelPrinter labelHandler;
  AssignHandler assignHandler;

  // Find input variables from I/O APIs
  Finder.addMatcher(getcMatcher, &getcHandler);
  Finder.addMatcher(scanfMatcher, &scanfHandler);
  // Match key-points
  Finder.addMatcher(labelMatcher, &labelHandler);
  // To create input-dependence list (def-use) by traversing assign statements
  Finder.addMatcher(assignMatcher, &assignHandler);

  Tool.run(newFrontendActionFactory(&Finder).get());

  // llvm::outs() << "Input var list detected: ";
  // for(std::string var : inputVars)
  // {
  //    llvm::outs()<< var <<" ";
  // }
  // llvm::outs() <<"\n";

  // Check if input-dependence list created above influence key-points in the program 
  MatchFinder Finder2;
  VarRefHandler varRefHandler;
  Finder2.addMatcher(declRefExpr().bind("declrefexpr"), &varRefHandler);
  Tool.run(newFrontendActionFactory(&Finder2).get());

  // Look for deeper I/O semantics

  llvm::outs() << "\nLooking for deeper I/O semantics... \n";
  MatchFinder Finder3;
  EOFHandler eofHandler;
  Finder3.addMatcher(eofCheck, &eofHandler);
  Tool.run(newFrontendActionFactory(&Finder3).get());

  llvm::outs() << "\nStatic Analysis completed\n";

  return 0;
}
