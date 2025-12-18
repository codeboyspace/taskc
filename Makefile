.PHONY: all clean help demo

# Build target
all: focusc

# Compile with recommended flags
focusc: main.c
	gcc -Wall -Wextra -O2 main.c -o focusc

# Clean build artifacts
clean:
	rm -f focusc

# Run the program
run: focusc
	./focusc

# Show help
help:
	@echo "focusc - Focus & Time-Tracking Toolkit"
	@echo ""
	@echo "Usage:"
	@echo "  make              - Build focusc executable"
	@echo "  make run          - Build and run focusc"
	@echo "  make clean        - Remove focusc executable"
	@echo "  make demo         - Show demo output"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Manual build:"
	@echo "  gcc -Wall -Wextra -O2 main.c -o focusc"
	@echo ""
	@echo "Run:"
	@echo "  ./focusc"

# Demo
demo:
	@bash demo.sh
