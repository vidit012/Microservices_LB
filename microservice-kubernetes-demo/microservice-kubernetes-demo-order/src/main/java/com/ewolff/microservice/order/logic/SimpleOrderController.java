package com.ewolff.microservice.order.logic;

import java.util.Collection;

import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.stereotype.Controller;
import org.springframework.web.bind.annotation.ModelAttribute;
import org.springframework.web.bind.annotation.PathVariable;
import org.springframework.web.bind.annotation.RequestMapping;
import org.springframework.web.bind.annotation.RequestMethod;
import org.springframework.web.bind.annotation.RequestParam;
import org.springframework.web.servlet.ModelAndView;

import com.ewolff.microservice.order.clients.CatalogClient;
import com.ewolff.microservice.order.clients.Customer;
import com.ewolff.microservice.order.clients.CustomerClient;
import com.ewolff.microservice.order.clients.Item;

@Controller
class SimpleOrderController {

	private SimpleOrderService simpleOrderService;
	private CustomerClient customerClient;
	private CatalogClient catalogClient;

	@Autowired
	public SimpleOrderController(SimpleOrderService simpleOrderService,
			CustomerClient customerClient, CatalogClient catalogClient) {
		super();
		this.simpleOrderService = simpleOrderService;
		this.customerClient = customerClient;
		this.catalogClient = catalogClient;
	}

	@ModelAttribute("items")
	public Collection<Item> items() {
		return catalogClient.findAll();
	}

	@ModelAttribute("customers")
	public Collection<Customer> customers() {
		return customerClient.findAll();
	}

	@RequestMapping("/")
	public ModelAndView orderList() {
		return new ModelAndView("simpleOrderList", "orders",
				simpleOrderService.findAll());
	}

	@RequestMapping(value = "/form.html", method = RequestMethod.GET)
	public ModelAndView form() {
		return new ModelAndView("simpleOrderForm");
	}

	@RequestMapping(value = "/{id}", method = RequestMethod.GET)
	public ModelAndView get(@PathVariable("id") long id) {
		return new ModelAndView("simpleOrder", "order", 
				simpleOrderService.findById(id));
	}

	@RequestMapping(value = "/", method = RequestMethod.POST)
	public ModelAndView post(
			@RequestParam("customerId") String customerId,
			@RequestParam("itemId") long itemId,
			@RequestParam("quantity") int quantity) {
		try {
			simpleOrderService.createOrder(customerId, itemId, quantity);
			return new ModelAndView("success");
		} catch (IllegalArgumentException e) {
			return new ModelAndView("error", "message", e.getMessage());
		}
	}

	@RequestMapping(value = "/{id}", method = RequestMethod.DELETE)
	public ModelAndView delete(@PathVariable("id") long id) {
		simpleOrderService.deleteById(id);
		return new ModelAndView("success");
	}
}
