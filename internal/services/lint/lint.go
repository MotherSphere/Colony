package lint

import (
	"bytes"
	"context"
	"errors"
	"os/exec"
)

func Run(ctx context.Context, dir string, command []string) (string, error) {
	if len(command) == 0 {
		return "", errors.New("no lint command configured")
	}
	cmd := exec.CommandContext(ctx, command[0], command[1:]...)
	cmd.Dir = dir
	var output bytes.Buffer
	cmd.Stdout = &output
	cmd.Stderr = &output
	err := cmd.Run()
	return output.String(), err
}
