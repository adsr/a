class Seq7f {

  constructor(renderAtId) {
    this.renderAtId = renderAtId;
  }

  run() {
    this.render();
  }

  render() {
    ReactDOM.render(
      <Sequencer seq={this} />,
      document.getElementById(this.renderAtId)
    );
  }

}
