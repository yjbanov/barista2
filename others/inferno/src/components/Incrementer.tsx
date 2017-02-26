import Component from 'inferno-component';

interface Props {
	name: string;
}

export class Incrementer extends Component<Props, any> {
	public state: {value: number};

	constructor(props, context) {
		super(props, context);

		this.state = {
			value: 1
		};
	}

	doMath = () => {
		this.setState({
			value: this.state.value + 1
		});
	}

	render() {
		return (
			<div>
				{this.props.name}
				<button onClick={this.doMath}>Increment</button>
				<div>{this.state.value + ' foobar'}</div>
			</div>
		);
	}
}
